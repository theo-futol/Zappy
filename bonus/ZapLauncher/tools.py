import re
from multiagent import Agent


def _clean_look(raw: str) -> str:
    """Strip zero-count items and label each tile explicitly."""
    stripped = re.sub(r'\w+:0 ', '', raw)
    # stripped looks like "[contents, contents, ...]"
    inner = stripped.strip().lstrip('[').rstrip(']')
    parts = inner.split(', ')
    labeled = []
    for i, part in enumerate(parts):
        content = part.strip()
        labeled.append(f"tile{i}: {content if content else '(empty)'}")
    return '[' + ' | '.join(labeled) + ']'


def forward(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Forward")
    return "ok (no server)"


def left(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Left")
    return "ok (no server)"


def right(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Right")
    return "ok (no server)"


def look(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return _clean_look(agent.connection.send_command("Look"))
    return "[player , , , ]"


def inventory(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Inventory")
    return "[food 0, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0] (no server)"


def broadcast(agent: Agent, args: str = "") -> str:
    text = args.strip()
    if not text:
        return "error: broadcast requires a message on the same line — USE broadcast <your message here>"
    # The server parses the Broadcast argument as a single whitespace-delimited token,
    # so multi-word messages must have spaces replaced before sending.
    token = text.replace(" ", "_")
    if agent.connection is not None:
        return agent.connection.send_command(f"Broadcast {token}")
    return "ok (no server)"


def connect_nbr(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Connect_nbr")
    return "0 (no server)"


def fork(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Fork")
    return "ok (no server)"


def eject(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Eject")
    return "ok (no server)"


def take(agent: Agent, args: str = "") -> str:
    obj = args.strip()
    if not obj:
        return "ko: take requires an object name (food, linemate, deraumere, sibur, mendiane, phiras, thystame)"
    if agent.connection is not None:
        result = agent.connection.send_command(f"Take {obj}")
        if result.strip() == "ko":
            return "ko: nothing to take here — that resource is on a different tile. Navigate there first (forward/left/right), then take."
        return result
    return "ok (no server)"


def set_object(agent: Agent, args: str = "") -> str:
    obj = args.strip()
    if not obj:
        return "[error] Set requires an object name"
    if agent.connection is not None:
        return agent.connection.send_command(f"Set {obj}")
    return "ok (no server)"


def incantation(agent: Agent, args: str = "") -> str:
    if agent.connection is not None:
        return agent.connection.send_command("Incantation")
    return "Elevation underway (no server)"


TOOLS = {
    "forward": forward,
    "left": left,
    "right": right,
    "look": look,
    "inventory": inventory,
    "broadcast": broadcast,
    "connect_nbr": connect_nbr,
    "fork": fork,
    "eject": eject,
    "take": take,
    "set": set_object,
    "incantation": incantation,
}


def use_tool(tool_name: str, tool_args: str, agent: Agent) -> str:
    """
    Run the tool named `tool_name` with `agent`, per the USE/REPORT protocol.
    `tool_args` is the remainder of the USE line after the tool name.
    """
    if tool_name not in TOOLS:
        raise ValueError(f"Unknown tool: {tool_name}")
    try:
        return TOOLS[tool_name](agent, tool_args)
    except ConnectionError:
        agent.connection = None
        return "[dead] Disconnected — this agent has died on the server."
