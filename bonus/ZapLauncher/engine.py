import os
import re
import time
import uuid
from openai import OpenAI

from multiagent import Team, Agent, HistoryEntry, Message
import runtime
import tools

API_KEY = os.getenv("API_KEY")

client = OpenAI(
    api_key=API_KEY,
    base_url="https://api.mistral.ai/v1",
)

# How many recent history entries to include as conversation context
_HISTORY_WINDOW = 60


def _find_commands(answer: str) -> list[tuple[str, str]]:
    """Return all (USE|REPORT, payload) pairs found in the answer, in order."""
    commands = []
    for line in answer.splitlines():
        m = re.match(r"^(USE|REPORT)\s+(.*)", line.strip())
        if m:
            commands.append((m.group(1), m.group(2).strip()))
    return commands


def _history_to_messages(history: list[HistoryEntry]) -> list[Message]:
    """Convert stored history entries to LLM conversation messages."""
    messages: list[Message] = []
    for entry in history:
        kind = entry.get("kind")
        content = entry.get("content") or ""
        if kind == "user":
            messages.append({"role": "user", "content": content})
        elif kind == "tool_use":
            tool_input = entry.get("tool_input") or entry.get("tool_name") or ""
            messages.append({"role": "assistant", "content": f"USE {tool_input}"})
        elif kind == "tool_result":
            messages.append({"role": "user", "content": f"[Environment]: {content}"})
        elif kind == "report" and not entry.get("truncated"):
            prefix = "" if entry.get("malformed") else "REPORT "
            messages.append({"role": "assistant", "content": f"{prefix}{content}"})
    return messages


def _call_llm(team: Team, messages: list[Message]) -> tuple[str, str]:
    response = client.chat.completions.create(
        model=team["model"],
        extra_body={"prompt_cache_key": team["prompt_cache_key"]},
        messages=messages,
    )
    message_id = uuid.uuid4().hex
    runtime.usage_log[message_id] = response.usage.model_dump() if response.usage else {}
    return response.choices[0].message.content, message_id


def run_turn(
    team: Team,
    agent: Agent,
    user_message: str,
    max_steps: int = 8,
    autonomous: bool = False,
) -> list[HistoryEntry]:
    """
    Drive one turn to completion using the USE/REPORT protocol.
    The LLM receives the full recent conversation history on every call,
    so it retains context across both tool steps within a turn and across turns.
    """
    new_entries: list[HistoryEntry] = []

    def log(entry: HistoryEntry) -> HistoryEntry:
        entry["timestamp"] = time.time()
        agent.history.append(entry)
        new_entries.append(entry)
        return entry

    log({"kind": "user", "content": user_message})

    # Build the base: system prompts + recent history (before this turn's user entry)
    system_messages: list[Message] = [
        {"role": "system", "content": agent.cached_prompt},
        {"role": "system", "content": agent.personality_prompt},
        {
            "role": "system",
            "content": (
                f"Your name is {agent.id}. Your team is {agent.team_id}.\n"
                "Always sign your broadcasts with your name and team so teammates can identify you.\n"
                "When you read a broadcast, try to infer the sender's team from name or phrasing "
                "— messages from your own team deserve a different response than enemy chatter."
            ),
        },
    ]
    prior = agent.history[:-1]
    if len(prior) > _HISTORY_WINDOW:
        prior = prior[-_HISTORY_WINDOW:]
    history_messages = _history_to_messages(prior)

    trigger: Message = (
        {"role": "system", "content": user_message}
        if autonomous
        else {"role": "user", "content": f"[Commander]: {user_message}"}
    )

    # This list grows as the turn progresses
    messages: list[Message] = system_messages + history_messages + [trigger]

    for _ in range(max_steps):
        answer, message_id = _call_llm(team, messages)

        commands = _find_commands(answer)
        if not commands:
            log({"kind": "llm_response", "content": answer, "message_id": message_id, "malformed": True})
            return new_entries

        # Log the full raw LLM response as one entry for the dashboard conversation view.
        # Individual tool_use entries that follow are used only for LLM context reconstruction.
        log({"kind": "llm_response", "content": answer, "message_id": message_id})

        done = False
        for i, (command, payload) in enumerate(commands):
            cmd_message_id = message_id if i == 0 else None

            if command == "REPORT":
                log({"kind": "report", "content": payload, "message_id": cmd_message_id})
                messages.append({"role": "assistant", "content": f"REPORT {payload}"})
                done = True
                break

            parts = payload.split(maxsplit=1)
            tool_name = parts[0] if parts else ""
            tool_args = parts[1] if len(parts) > 1 else ""
            log({"kind": "tool_use", "tool_name": tool_name, "tool_input": payload, "message_id": cmd_message_id})
            messages.append({"role": "assistant", "content": f"USE {payload}"})

            try:
                result_str = tools.use_tool(tool_name, tool_args, agent)
            except ValueError as e:
                result_str = str(e)

            # Deliver any broadcast messages that arrived while this command was running
            if agent.connection is not None:
                received = agent.connection.drain_messages()
                if received:
                    result_str += "\n" + "\n".join(received)

            log({"kind": "tool_result", "tool_name": tool_name, "content": result_str})
            messages.append({"role": "user", "content": f"[Environment]: {result_str}"})

        if done:
            return new_entries

    return new_entries
