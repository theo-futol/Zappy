"use strict";

// When served by the API itself (e.g. at /leader), default to the same origin;
// when opened as a local file, fall back to the usual localhost API.
const defaultApiBase = window.location.protocol.startsWith("http")
  ? window.location.origin
  : "http://localhost:8000";

const state = {
  apiBase: localStorage.getItem("zap.apiBase") || defaultApiBase,
  teams: [],
  selectedTeamId: null,
  selectedAgentId: null,
  agentCache: {}, // agentId -> { description, conversation, historyLength }
  editing: false,
  connStatus: { server_connected: false, agents: {} },
  conversationTab: "conversation",
  feedVisible: localStorage.getItem("zap.feedVisible") !== "false",
  feedLastSince: Date.now() / 1000 - 60, // last 60 seconds on first load
  lastActive: {}, // agentId -> timestamp of last feed entry
};

// ---------------------------------------------------------------------------
// API helper
// ---------------------------------------------------------------------------

async function api(path, options = {}) {
  const res = await fetch(state.apiBase + path, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });

  if (!res.ok) {
    let detail = res.statusText;
    try {
      const body = await res.json();
      detail = body.detail || JSON.stringify(body);
    } catch (_) {}
    throw new Error(`${res.status}: ${detail}`);
  }

  if (res.status === 204) return null;
  return res.json();
}

function stripEmpty(obj) {
  const out = {};
  for (const [k, v] of Object.entries(obj)) {
    if (v !== "" && v !== null && v !== undefined) out[k] = v;
  }
  return out;
}

// ---------------------------------------------------------------------------
// Toast
// ---------------------------------------------------------------------------

let toastTimer = null;

function toast(message, isError = false) {
  const el = document.getElementById("toast");
  el.textContent = message;
  el.classList.toggle("error", isError);
  el.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove("show"), 3500);
}

// ---------------------------------------------------------------------------
// Modal
// ---------------------------------------------------------------------------

function openModal(html) {
  document.getElementById("modal").innerHTML = html;
  document.getElementById("modal-overlay").classList.remove("hidden");
}

function closeModal() {
  document.getElementById("modal-overlay").classList.add("hidden");
  document.getElementById("modal").innerHTML = "";
}

function modalError(message) {
  let box = document.getElementById("modal-error");
  if (!box) {
    box = document.createElement("div");
    box.id = "modal-error";
    box.className = "modal-error";
    const actions = document.querySelector("#modal .modal-actions");
    if (actions) actions.before(box);
    else document.getElementById("modal").appendChild(box);
  }
  box.textContent = message;
  box.classList.remove("hidden");
}

function clearModalError() {
  const box = document.getElementById("modal-error");
  if (box) box.classList.add("hidden");
}

function friendlyApiError(err, context = "") {
  const msg = err.message || String(err);
  const match = msg.match(/^(\d+):\s*(.*)/s);
  if (!match) return context ? `${context}: ${msg}` : msg;
  const [, code, detail] = match;
  switch (code) {
    case "409": return `Already exists: ${detail.replace(/^[^:]+:\s*/, "")}`;
    case "503": return `Server unavailable — ${detail}`;
    case "404": return `Not found: ${detail.replace(/^[^:]+:\s*/, "")}`;
    case "422": return `Invalid request: ${detail}`;
    default:    return detail || msg;
  }
}

document.getElementById("modal-overlay").addEventListener("click", (e) => {
  if (e.target.id === "modal-overlay") closeModal();
});
window.addEventListener("keydown", (e) => {
  if (e.key === "Escape") closeModal();
});

// ---------------------------------------------------------------------------
// Connection dots
// ---------------------------------------------------------------------------

function setDot(el, status) {
  if (!el) return;
  el.className = `dot ${status}`;
  el.title = status;
}

function applyStatus(data) {
  const prev = state.connStatus?.agents || {};
  state.connStatus = data;
  const apiStatus = data.server_connected ? "connected" : "waiting";
  setDot(document.getElementById("api-dot"), apiStatus);
  const busySet = new Set(data.busy_agents || []);
  let agentDied = false;
  for (const [id, s] of Object.entries(data.agents)) {
    // Show "thinking" when an LLM turn is running, regardless of connection status.
    const dotStatus = busySet.has(id) ? "thinking" : s;
    setDot(document.querySelector(`.dot[data-agent-id="${CSS.escape(id)}"]`), dotStatus);
    if (prev[id] === "connected" && s === "disconnected") agentDied = true;
  }
  if (agentDied) loadTeams();
}

async function loadStatus() {
  try {
    const data = await api("/status");
    applyStatus(data);
  } catch (_) {
    setDot(document.getElementById("api-dot"), "disconnected");
    document.querySelectorAll(".dot[data-agent-id]").forEach((d) =>
      setDot(d, "disconnected")
    );
  }
}

// ---------------------------------------------------------------------------
// Teams / tabs
// ---------------------------------------------------------------------------

async function loadTeams() {
  try {
    state.teams = await api("/teams");
  } catch (e) {
    toast(`Could not reach API: ${e.message}`, true);
    state.teams = [];
  }

  if (!state.teams.find((t) => t.id === state.selectedTeamId)) {
    state.selectedTeamId = state.teams[0]?.id || null;
    state.selectedAgentId = null;
  }

  renderTabs();
  renderSidebar();
  if (!state.selectedAgentId) showEmptyState();
}

function teamSlotLabel(team) {
  if (team.slots_available !== null && team.slots_available !== undefined &&
      team.slots_total    !== null && team.slots_total    !== undefined) {
    const used = team.slots_total - team.slots_available;
    return `${used} / ${team.slots_total}`;
  }
  return null;
}

function renderTabs() {
  const nav = document.getElementById("team-tabs");
  nav.innerHTML = "";

  for (const team of state.teams) {
    const tab = document.createElement("div");
    tab.className = "tab" + (team.id === state.selectedTeamId ? " active" : "");
    tab.onclick = () => selectTeam(team.id);

    const nameSpan = document.createElement("span");
    nameSpan.textContent = team.id;
    tab.appendChild(nameSpan);

    const slotLabel = teamSlotLabel(team);
    if (slotLabel) {
      tab.appendChild(document.createTextNode(" " + slotLabel));
    }

    nav.appendChild(tab);
  }
}

function selectTeam(teamId) {
  state.selectedTeamId = teamId;
  state.selectedAgentId = null;
  renderTabs();
  renderSidebar();
  showEmptyState();
}

function currentTeam() {
  return state.teams.find((t) => t.id === state.selectedTeamId) || null;
}

// ---------------------------------------------------------------------------
// Sidebar / agents
// ---------------------------------------------------------------------------

function renderSidebar() {
  const list = document.getElementById("agent-list");
  list.innerHTML = "";

  const team = currentTeam();
  document.getElementById("btn-new-agent").disabled = !team;
  document.getElementById("btn-configure-team").disabled = !team;

  if (!team) return;

  for (const agentId of team.agent_ids) {
    const li = document.createElement("li");
    li.className = agentId === state.selectedAgentId ? "active" : "";
    li.onclick = () => selectAgent(agentId);

    const dot = document.createElement("span");
    dot.className = `dot ${state.connStatus.agents[agentId] || "waiting"}`;
    dot.dataset.agentId = agentId;
    li.appendChild(dot);

    const nameSpan = document.createElement("span");
    nameSpan.className = "agent-name";
    nameSpan.textContent = " " + agentId;
    li.appendChild(nameSpan);

    const ts = state.lastActive[agentId];
    if (ts) {
      const timer = document.createElement("span");
      timer.className = "agent-timer";
      timer.dataset.ts = ts;
      timer.textContent = formatRelTime(ts);
      li.appendChild(timer);
    }

    list.appendChild(li);
  }
}

function showEmptyState() {
  document.getElementById("empty-state").classList.remove("hidden");
  document.getElementById("agent-view").classList.add("hidden");
}

async function selectAgent(agentId) {
  state.selectedAgentId = agentId;
  state.editing = false;
  renderSidebar();

  try {
    const [description, conversation] = await Promise.all([
      api(`/agents/${encodeURIComponent(agentId)}`),
      api(`/agents/${encodeURIComponent(agentId)}/conversation`),
    ]);
    state.agentCache[agentId] = { description, conversation, historyLength: conversation.length };
  } catch (e) {
    toast(`Failed to load agent: ${e.message}`, true);
    return;
  }

  renderAgentView();
}

function renderAgentView() {
  document.getElementById("empty-state").classList.add("hidden");
  document.getElementById("agent-view").classList.remove("hidden");

  const { description, conversation } = state.agentCache[state.selectedAgentId];

  document.getElementById("agent-title").textContent = description.id;
  document.getElementById("agent-team").textContent = `team: ${description.team_id}`;

  const personalityBox = document.getElementById("personality-prompt");
  const cachedBox = document.getElementById("cached-prompt");
  personalityBox.value = description.personality_prompt;
  cachedBox.value = description.cached_prompt;

  renderMemory(description);
  setEditMode(false);
  renderConversation(conversation);
}

function renderMemory(description) {
  const list = document.getElementById("agent-memory");
  list.innerHTML = "";
  const facts = description.facts_memory || [];
  if (!facts.length) {
    const li = document.createElement("li");
    li.style.listStyle = "none";
    li.className = "muted";
    li.textContent = "(nothing remembered yet)";
    list.appendChild(li);
    return;
  }
  for (const fact of facts) {
    const li = document.createElement("li");
    li.textContent = fact;
    list.appendChild(li);
  }
}

document.getElementById("toggle-memory").onclick = () => {
  const box = document.getElementById("agent-memory");
  const btn = document.getElementById("toggle-memory");
  const willShow = box.classList.contains("hidden");
  box.classList.toggle("hidden");
  btn.textContent = (willShow ? "▾" : "▸") + " Memory (facts the agent chose to remember)";
};

function setEditMode(on) {
  state.editing = on;
  document.getElementById("personality-prompt").readOnly = !on;
  document.getElementById("cached-prompt").readOnly = !on;
  document.getElementById("edit-actions").classList.toggle("hidden", !on);
  document.getElementById("btn-edit-agent").classList.toggle("hidden", on);
  if (on) {
    document.getElementById("cached-prompt").classList.remove("hidden");
    document.getElementById("toggle-cached").textContent = "▾ Cached prompt (protocol + lore + team)";
  }
}

document.getElementById("btn-edit-agent").onclick = () => setEditMode(true);

document.getElementById("btn-cancel-edit").onclick = () => {
  const { description } = state.agentCache[state.selectedAgentId];
  document.getElementById("personality-prompt").value = description.personality_prompt;
  document.getElementById("cached-prompt").value = description.cached_prompt;
  setEditMode(false);
};

document.getElementById("btn-save-agent").onclick = async () => {
  const agentId = state.selectedAgentId;
  const personality_prompt = document.getElementById("personality-prompt").value;
  const cached_prompt = document.getElementById("cached-prompt").value;

  try {
    const description = await api(`/agents/${encodeURIComponent(agentId)}`, {
      method: "PATCH",
      body: JSON.stringify({ personality_prompt, cached_prompt }),
    });
    state.agentCache[agentId].description = description;
    setEditMode(false);
    toast("Agent prompts updated.");
  } catch (e) {
    toast(`Failed to update agent: ${e.message}`, true);
  }
};

document.getElementById("toggle-cached").onclick = () => {
  if (state.editing) return;
  const box = document.getElementById("cached-prompt");
  const btn = document.getElementById("toggle-cached");
  const willShow = box.classList.contains("hidden");
  box.classList.toggle("hidden");
  btn.textContent = (willShow ? "▾" : "▸") + " Cached prompt (protocol + lore + team)";
};

// ---------------------------------------------------------------------------
// Conversation
// ---------------------------------------------------------------------------

function entryLabel(entry) {
  switch (entry.kind) {
    case "user":          return "User";
    case "llm_response":  return entry.malformed ? "LLM (malformed)" : "LLM";
    case "tool_use":      return `USE ${entry.tool_name || ""}`;
    case "tool_result":   return `RESULT (${entry.tool_name || ""})`;
    case "report":        return entry.malformed ? "REPORT (malformed)" : "REPORT";
    default:              return entry.kind;
  }
}

function entryBodyText(entry) {
  if (entry.kind === "tool_use") return entry.tool_name ? `USE ${entry.tool_name}` : "USE";
  return entry.content || "";
}

function shouldShowEntry(entry, isLogs) {
  if (isLogs) {
    // Logs: raw debug view — hide llm_response (tool_use/report already cover it)
    return entry.kind !== "llm_response";
  } else {
    // Conversation: hide tool_use and report, shown as one llm_response bubble instead
    return entry.kind !== "tool_use" && entry.kind !== "report";
  }
}

function buildEntryElement(entry, index) {
  const div = document.createElement("div");
  div.className = `entry ${entry.kind}` + (entry.malformed ? " malformed" : "");
  const label = document.createElement("span");
  label.className = "kind-label";
  label.textContent = entryLabel(entry);
  div.appendChild(label);
  div.appendChild(document.createTextNode(entryBodyText(entry)));
  if (entry.timestamp) {
    const ts = document.createElement("span");
    ts.className = "entry-ts";
    ts.dataset.ts = entry.timestamp;
    ts.textContent = formatRelTime(entry.timestamp);
    div.appendChild(ts);
  }
  div.onclick = () => openMessageDetail(index);
  return div;
}

function logSource(entry) {
  switch (entry.kind) {
    case "user":          return "User";
    case "llm_response":  return "LLM";
    case "tool_use":      return "LLM";
    case "tool_result":   return entry.tool_name || "tool";
    case "report":        return "LLM";
    default:              return entry.kind;
  }
}

function buildLogRow(entry, index) {
  const row = document.createElement("div");
  row.className = `log-row ${entry.kind}` + (entry.malformed ? " malformed" : "");
  row.onclick = () => openMessageDetail(index);

  const src = document.createElement("span");
  src.className = "log-source";
  src.textContent = logSource(entry);

  const body = document.createElement("span");
  body.className = "log-content";
  if (entry.kind === "tool_use") {
    body.textContent = `USE ${entry.tool_name || ""}`;
  } else if (entry.kind === "report") {
    body.textContent = `REPORT ${entry.content || ""}`;
  } else {
    body.textContent = entry.content || "";
  }

  const ts = document.createElement("span");
  ts.className = "log-ts";
  if (entry.timestamp) {
    ts.dataset.ts = entry.timestamp;
    ts.textContent = formatRelTime(entry.timestamp);
  }

  row.appendChild(src);
  row.appendChild(body);
  row.appendChild(ts);
  return row;
}

function buildElement(entry, index) {
  return state.conversationTab === "logs"
    ? buildLogRow(entry, index)
    : buildEntryElement(entry, index);
}

function renderConversation(entries) {
  const box = document.getElementById("conversation");
  box.innerHTML = "";
  const isLogs = state.conversationTab === "logs";
  box.classList.toggle("logs-mode", isLogs);
  entries.forEach((entry, index) => {
    if (!shouldShowEntry(entry, isLogs)) return;
    box.appendChild(buildElement(entry, index));
  });
  box.scrollTop = box.scrollHeight;
}

function appendConversationEntries(agentId, newEntries) {
  const cache = state.agentCache[agentId];
  const startIndex = cache.conversation.length;
  cache.conversation.push(...newEntries);
  cache.historyLength = cache.conversation.length;

  // Memory changed? Refetch the description so the memory panel stays current.
  if (newEntries.some((e) => e.kind === "tool_use" && (e.tool_name === "remember" || e.tool_name === "forget"))) {
    api(`/agents/${encodeURIComponent(agentId)}`).then((description) => {
      cache.description = description;
      if (state.selectedAgentId === agentId) renderMemory(description);
    }).catch(() => {});
  }

  if (state.selectedAgentId !== agentId) return;
  const box = document.getElementById("conversation");
  const isLogs = state.conversationTab === "logs";
  const wasAtBottom = box.scrollHeight - box.clientHeight <= box.scrollTop + 20;
  newEntries.forEach((entry, i) => {
    if (!shouldShowEntry(entry, isLogs)) return;
    box.appendChild(buildElement(entry, startIndex + i));
  });
  if (wasAtBottom) box.scrollTop = box.scrollHeight;
}

document.querySelectorAll(".conv-tab").forEach((btn) => {
  btn.addEventListener("click", () => {
    if (btn.dataset.tab === state.conversationTab) return;
    state.conversationTab = btn.dataset.tab;
    document.querySelectorAll(".conv-tab").forEach((b) => b.classList.toggle("active", b === btn));
    if (state.selectedAgentId && state.agentCache[state.selectedAgentId]) {
      renderConversation(state.agentCache[state.selectedAgentId].conversation);
    }
  });
});

function openMessageDetail(index) {
  const agentId = state.selectedAgentId;
  const { description, conversation } = state.agentCache[agentId];
  const entry = conversation[index];

  const hasUsage = Boolean(entry.message_id);
  const messageSent = entry.kind === "user" ? entry.content : (conversation[index - 1]?.content || "");

  const fullPrompt = [description.cached_prompt, description.personality_prompt, "--- message ---", messageSent]
    .join("\n\n");

  openModal(`
    <h3>${entryLabel(entry)}</h3>
    <div class="field">
      <label>Content</label>
      <textarea class="prompt-text" readonly>${escapeHtml(entry.content || "")}</textarea>
    </div>
    ${hasUsage ? `<div id="usage-box"><div class="field"><label>Token usage</label><div class="muted">Loading...</div></div></div>` : ""}
    <div class="field">
      <label>Full prompt sent to the LLM (including cached part)</label>
      <textarea class="prompt-text" style="min-height:220px" readonly>${escapeHtml(fullPrompt)}</textarea>
    </div>
    <div class="modal-actions">
      <button class="btn ghost" id="modal-close">Close</button>
    </div>
  `);

  document.getElementById("modal-close").onclick = closeModal;
  if (hasUsage) loadUsage(entry.message_id);
}

async function loadUsage(messageId) {
  const box = document.getElementById("usage-box");
  if (!box) return;
  try {
    const data = await api(`/usage/${encodeURIComponent(messageId)}`);
    const rows = Object.entries(data.usage)
      .map(([k, v]) => `<div><div class="label">${escapeHtml(k)}</div>${formatUsageValue(v)}</div>`)
      .join("");
    box.innerHTML = `<div class="field"><label>Token usage (message ${escapeHtml(messageId)})</label><div class="usage-grid">${rows}</div></div>`;
  } catch (e) {
    box.innerHTML = `<div class="field"><label>Token usage</label><div class="muted">Unavailable: ${escapeHtml(e.message)}</div></div>`;
  }
}

function formatUsageValue(v) {
  if (v === null || v === undefined) return "—";
  if (typeof v === "object") return `<pre style="margin:0;white-space:pre-wrap">${escapeHtml(JSON.stringify(v, null, 2))}</pre>`;
  return String(v);
}

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

// ---------------------------------------------------------------------------
// Relative time
// ---------------------------------------------------------------------------

function formatRelTime(ts) {
  if (!ts) return "";
  const diff = Date.now() / 1000 - ts;
  if (diff < 5)   return "just now";
  if (diff < 60)  return `${Math.round(diff)}s ago`;
  if (diff < 3600) return `${Math.round(diff / 60)}m ago`;
  return `${Math.round(diff / 3600)}h ago`;
}

function tickRelTimes() {
  document.querySelectorAll("[data-ts]").forEach((el) => {
    el.textContent = formatRelTime(parseFloat(el.dataset.ts));
  });
}

// ---------------------------------------------------------------------------
// Live feed panel
// ---------------------------------------------------------------------------

function applyFeedVisibility() {
  const panel = document.getElementById("feed-panel");
  const btn = document.getElementById("btn-toggle-feed");
  panel.classList.toggle("collapsed", !state.feedVisible);
  btn.classList.toggle("active", state.feedVisible);
  btn.textContent = state.feedVisible ? "Hide Feed" : "Live Feed";
}

document.getElementById("btn-toggle-feed").onclick = () => {
  state.feedVisible = !state.feedVisible;
  localStorage.setItem("zap.feedVisible", state.feedVisible);
  applyFeedVisibility();
};

document.getElementById("btn-clear-feed").onclick = () => {
  document.getElementById("feed-list").innerHTML = "";
};

function feedKindLabel(entry) {
  switch (entry.kind) {
    case "llm_response": return entry.content ? entry.content.split("\n")[0] : "LLM";
    case "tool_use":     return `USE ${entry.tool_name || ""}`;
    case "tool_result":  return entry.tool_name || "result";
    case "user":         return `→ ${entry.content || ""}`;
    case "report":       return `REPORT`;
    default:             return entry.kind;
  }
}

function buildFeedEntry(entry) {
  // In the feed, hide tool_use and report — they are already represented by llm_response
  if (entry.kind === "tool_use" || entry.kind === "report") return null;
  const el = document.createElement("div");
  el.className = `feed-entry feed-kind-${entry.kind}`;

  const header = document.createElement("div");
  header.className = "feed-entry-header";

  const agentSpan = document.createElement("span");
  agentSpan.className = "feed-agent";
  agentSpan.textContent = entry.agent_id;

  const timeSpan = document.createElement("span");
  timeSpan.className = "feed-time";
  if (entry.timestamp) {
    timeSpan.dataset.ts = entry.timestamp;
    timeSpan.textContent = formatRelTime(entry.timestamp);
  }

  header.appendChild(agentSpan);
  header.appendChild(timeSpan);
  el.appendChild(header);

  const action = document.createElement("div");
  action.className = "feed-action";
  action.textContent = feedKindLabel(entry);
  el.appendChild(action);

  if (entry.kind === "tool_result" && entry.content) {
    const result = document.createElement("div");
    result.className = "feed-result";
    result.textContent = entry.content;
    el.appendChild(result);
  }

  if (entry.kind === "report" && entry.content) {
    const body = document.createElement("div");
    body.className = "feed-result";
    body.textContent = entry.content;
    el.appendChild(body);
  }

  // Click to jump to that agent
  el.onclick = () => {
    const team = state.teams.find((t) =>
      t.agent_ids && t.agent_ids.includes(entry.agent_id)
    );
    if (team) {
      if (state.selectedTeamId !== team.id) {
        state.selectedTeamId = team.id;
        renderTabs();
        renderSidebar();
      }
      selectAgent(entry.agent_id);
    }
  };

  return el;
}

async function loadFeed() {
  try {
    const newEntries = await api(`/feed?since=${state.feedLastSince}`);
    if (!newEntries.length) return;

    const maxTs = Math.max(...newEntries.map((e) => e.timestamp || 0));
    if (maxTs > state.feedLastSince) state.feedLastSince = maxTs;

    // Update last-active timestamps
    const affectedAgents = new Set();
    for (const entry of newEntries) {
      if (entry.timestamp && entry.timestamp > (state.lastActive[entry.agent_id] || 0)) {
        state.lastActive[entry.agent_id] = entry.timestamp;
        affectedAgents.add(entry.agent_id);
      }
    }

    // Update sidebar timers for affected agents without full re-render
    for (const agentId of affectedAgents) {
      const timerEl = document.querySelector(`.agent-timer[data-agent-id="${CSS.escape(agentId)}"]`);
      if (timerEl) {
        timerEl.dataset.ts = state.lastActive[agentId];
        timerEl.textContent = formatRelTime(state.lastActive[agentId]);
      } else {
        // Insert timer if missing (agent was rendered before we had data)
        const li = document.querySelector(`#agent-list li.active, #agent-list li`);
        // Re-render sidebar so the timer appears
        renderSidebar();
        break;
      }
    }

    // Render new feed entries
    if (state.feedVisible) {
      const list = document.getElementById("feed-list");
      const wasAtBottom = list.scrollHeight - list.clientHeight <= list.scrollTop + 20;
      for (const entry of newEntries) {
        const el = buildFeedEntry(entry);
        if (el) list.appendChild(el);
      }
      if (wasAtBottom) list.scrollTop = list.scrollHeight;
      // Cap to 300 entries
      while (list.children.length > 300) list.removeChild(list.firstChild);
    }

    // Auto-refresh selected agent's conversation
    const selectedId = state.selectedAgentId;
    if (selectedId && affectedAgents.has(selectedId)) {
      await refreshSelectedConversation();
    }
  } catch (_) {}
}

async function refreshSelectedConversation() {
  const agentId = state.selectedAgentId;
  if (!agentId || !state.agentCache[agentId]) return;
  const since = state.agentCache[agentId].historyLength || 0;
  try {
    const newEntries = await api(
      `/agents/${encodeURIComponent(agentId)}/conversation?since=${since}`
    );
    if (newEntries.length) appendConversationEntries(agentId, newEntries);
  } catch (_) {}
}

// ---------------------------------------------------------------------------
// Prompt form
// ---------------------------------------------------------------------------

document.getElementById("prompt-form").addEventListener("submit", async (e) => {
  e.preventDefault();
  const input = document.getElementById("prompt-input");
  const message = input.value.trim();
  if (!message || !state.selectedAgentId) return;

  const agentId = state.selectedAgentId;
  input.value = "";
  const submitBtn = e.target.querySelector("button[type=submit]");
  submitBtn.disabled = true;

  try {
    const entries = await api(`/agents/${encodeURIComponent(agentId)}/prompt`, {
      method: "POST",
      body: JSON.stringify({ message }),
    });
    appendConversationEntries(agentId, entries);
  } catch (e2) {
    toast(`Prompt failed: ${e2.message}`, true);
  } finally {
    submitBtn.disabled = false;
  }
});

// ---------------------------------------------------------------------------
// Configure team
// ---------------------------------------------------------------------------

function openConfigureTeamModal() {
  const team = currentTeam();
  if (!team) return;

  openModal(`
    <h3>Configure "${escapeHtml(team.id)}"</h3>
    <div class="field">
      <label>Shared prompt</label>
      <textarea id="team-shared-prompt">${escapeHtml(team.shared_prompt || "")}</textarea>
    </div>
    <details class="advanced">
      <summary>Advanced (model / endpoint / cache key)</summary>
      <div class="field">
        <label>Model</label>
        <input type="text" id="team-model" value="${escapeHtml(team.model)}">
      </div>
      <div class="field">
        <label>Endpoint</label>
        <input type="text" id="team-endpoint" value="${escapeHtml(team.endpoint)}">
      </div>
      <div class="field">
        <label>Prompt cache key</label>
        <input type="text" id="team-cache-key" value="${escapeHtml(team.prompt_cache_key)}">
      </div>
    </details>
    <div class="modal-actions">
      <button class="btn ghost" id="modal-cancel">Cancel</button>
      <button class="btn" id="modal-submit">Save</button>
    </div>
  `);

  document.getElementById("modal-cancel").onclick = closeModal;
  document.getElementById("modal-submit").onclick = async () => {
    const payload = {
      shared_prompt: document.getElementById("team-shared-prompt").value,
      model: document.getElementById("team-model").value.trim() || team.model,
      endpoint: document.getElementById("team-endpoint").value.trim() || team.endpoint,
      prompt_cache_key: document.getElementById("team-cache-key").value.trim() || team.prompt_cache_key,
    };

    try {
      await api(`/teams/${encodeURIComponent(team.id)}`, {
        method: "PATCH",
        body: JSON.stringify(payload),
      });
      closeModal();
      await loadTeams();
      toast(`Team "${team.id}" updated.`);
    } catch (e) {
      modalError(friendlyApiError(e));
    }
  };
}

document.getElementById("btn-configure-team").onclick = openConfigureTeamModal;

// ---------------------------------------------------------------------------
// Create personality (standalone)
// ---------------------------------------------------------------------------

function openCreatePersonalityModal(onCreated) {
  openModal(`
    <h3>New personality</h3>
    <div class="field">
      <label>Name</label>
      <input type="text" id="personality-name" placeholder="e.g. healer">
    </div>
    <div class="field">
      <label>Content</label>
      <textarea id="personality-content" placeholder="You are a kind village healer..."></textarea>
    </div>
    <div class="modal-actions">
      <button class="btn ghost" id="modal-cancel">Cancel</button>
      <button class="btn" id="modal-submit">Create personality</button>
    </div>
  `);

  document.getElementById("modal-cancel").onclick = closeModal;
  document.getElementById("modal-submit").onclick = async () => {
    const name = document.getElementById("personality-name").value.trim();
    const content = document.getElementById("personality-content").value;

    if (!name || !content) {
      toast("Name and content are required.", true);
      return;
    }

    try {
      const personality = await api("/personalities", {
        method: "POST",
        body: JSON.stringify({ name, content }),
      });
      closeModal();
      toast(`Personality "${personality.name}" created.`);
      if (onCreated) onCreated(personality);
    } catch (e) {
      modalError(friendlyApiError(e));
    }
  };
}

document.getElementById("btn-new-personality").onclick = () => openCreatePersonalityModal();

// ---------------------------------------------------------------------------
// Create agent
// ---------------------------------------------------------------------------

async function openCreateAgentModal() {
  const team = currentTeam();
  if (!team) return;

  let personalities = [];
  try {
    personalities = await api("/prompts/personalities");
  } catch (e) {
    toast(`Failed to load personalities: ${e.message}`, true);
  }

  const options = personalities
    .map((p) => `<option value="${escapeHtml(p.name)}">${escapeHtml(p.name)}</option>`)
    .join("");

  const slotLabel = teamSlotLabel(team);
  const slotsHint = slotLabel ? ` ${slotLabel}` : "";
  const noSlots = team.slots_available === 0;

  openModal(`
    <h3>New agent in "${escapeHtml(team.id)}"${escapeHtml(slotsHint)}</h3>
    ${noSlots ? `<div class="modal-error">No slots available on the server for team "${escapeHtml(team.id)}". The agent cannot connect until a slot opens up.</div>` : ""}
    <div class="field">
      <label>Agent id (optional, defaults to personality name)</label>
      <input type="text" id="agent-id" placeholder="e.g. merchant2">
    </div>
    <div class="field-row" style="margin-bottom:12px">
      <input type="checkbox" id="agent-random">
      <label for="agent-random">Pick a random personality</label>
    </div>
    <div class="field" id="personality-field">
      <label>Personality</label>
      <select id="agent-personality">
        ${options}
        <option value="__new__">+ New personality...</option>
      </select>
    </div>
    <div class="field hidden" id="new-personality-fields">
      <label>New personality name</label>
      <input type="text" id="new-personality-name" placeholder="e.g. healer">
      <label>New personality content</label>
      <textarea id="new-personality-content" placeholder="You are a kind village healer..."></textarea>
    </div>
    <div class="modal-actions">
      <button class="btn ghost" id="modal-cancel">Cancel</button>
      <button class="btn" id="modal-submit">Create agent</button>
    </div>
  `);

  const randomBox = document.getElementById("agent-random");
  const personalityField = document.getElementById("personality-field");
  randomBox.onchange = () => personalityField.classList.toggle("hidden", randomBox.checked);

  const select = document.getElementById("agent-personality");
  const newFields = document.getElementById("new-personality-fields");
  select.onchange = () => newFields.classList.toggle("hidden", select.value !== "__new__");

  document.getElementById("modal-cancel").onclick = closeModal;
  document.getElementById("modal-submit").onclick = async () => {
    clearModalError();
    const id = document.getElementById("agent-id").value.trim();
    const random = randomBox.checked;

    let personality = select.value;

    try {
      if (!random && personality === "__new__") {
        const name = document.getElementById("new-personality-name").value.trim();
        const content = document.getElementById("new-personality-content").value;
        if (!name || !content) {
          modalError("New personality name and content are required.");
          return;
        }
        await api("/personalities", { method: "POST", body: JSON.stringify({ name, content }) });
        personality = name;
      }

      const payload = stripEmpty({
        team_id: team.id,
        id,
        personality: random ? "" : personality,
        random,
      });

      const agent = await api("/agents", { method: "POST", body: JSON.stringify(payload) });
      closeModal();
      await loadTeams();
      selectAgent(agent.id);
      toast(`Agent "${agent.id}" created.`);
    } catch (e) {
      modalError(friendlyApiError(e));
    }
  };
}

document.getElementById("btn-new-agent").onclick = openCreateAgentModal;

// ---------------------------------------------------------------------------
// Prompt database browser
// ---------------------------------------------------------------------------

const PROMPT_DB_TABS = ["teams", "personalities", "protocol", "trantorian"];

async function openPromptDatabaseModal(startTab = "teams") {
  openModal(`
    <h3>Prompt database</h3>
    <div class="inner-tabs" id="db-tabs"></div>
    <div id="db-body"></div>
    <div class="modal-actions">
      <button class="btn ghost" id="modal-close">Close</button>
    </div>
  `);
  document.getElementById("modal-close").onclick = closeModal;

  const tabsEl = document.getElementById("db-tabs");
  for (const name of PROMPT_DB_TABS) {
    const tab = document.createElement("div");
    tab.className = "inner-tab";
    tab.textContent = name;
    tab.onclick = () => showPromptDbTab(name);
    tabsEl.appendChild(tab);
  }

  showPromptDbTab(startTab);
}

async function showPromptDbTab(name) {
  document.querySelectorAll("#db-tabs .inner-tab").forEach((el) => {
    el.classList.toggle("active", el.textContent === name);
  });

  const body = document.getElementById("db-body");

  if (name === "trantorian") {
    body.innerHTML = `<div class="db-content">Loading...</div>`;
    try {
      const data = await api("/prompts/trantorian");
      body.innerHTML = `<div class="db-content">${escapeHtml(data.content)}</div>`;
    } catch (e) {
      body.innerHTML = `<div class="db-content">Failed: ${escapeHtml(e.message)}</div>`;
    }
    return;
  }

  if (name === "protocol") {
    body.innerHTML = `
      <div class="db-edit-panel">
        <textarea class="db-edit-area" id="db-edit-content" placeholder="Loading..."></textarea>
        <div class="db-save-row">
          <button class="btn small" id="db-save-btn">Save</button>
        </div>
      </div>`;
    try {
      const data = await api("/prompts/protocol");
      document.getElementById("db-edit-content").value = data.content;
    } catch (e) {
      toast(`Failed to load protocol: ${e.message}`, true);
    }
    document.getElementById("db-save-btn").addEventListener("click", async () => {
      const content = document.getElementById("db-edit-content").value;
      try {
        await api("/prompts/protocol", {
          method: "PUT",
          body: JSON.stringify({ name: "protocol", content }),
        });
        toast("Protocol prompt saved.");
      } catch (e) {
        toast(`Save failed: ${e.message}`, true);
      }
    });
    return;
  }

  const isTeams = name === "teams";
  body.innerHTML = `
    <div class="db-layout">
      <div class="db-list" id="db-list"></div>
      <div class="db-detail-col">
        <textarea class="db-edit-area" id="db-edit-content" placeholder="Select an item…" disabled></textarea>
        <div class="db-save-row">
          <span class="muted" id="db-selected-name"></span>
          ${!isTeams ? `<button class="btn small ghost" id="db-new-btn">+ New</button>` : ""}
          <button class="btn small" id="db-save-btn" disabled>Save</button>
        </div>
      </div>
    </div>`;

  let selectedName = null;

  const selectItem = (itemName, itemContent) => {
    document.querySelectorAll("#db-list .db-list-item").forEach((n) => n.classList.remove("active"));
    document.querySelector(`#db-list [data-name="${CSS.escape(itemName)}"]`)?.classList.add("active");
    selectedName = itemName;
    const ta = document.getElementById("db-edit-content");
    ta.value = itemContent;
    ta.disabled = false;
    document.getElementById("db-selected-name").textContent = itemName;
    document.getElementById("db-save-btn").disabled = false;
  };

  try {
    const items = isTeams ? await api("/teams") : await api("/prompts/personalities");
    const list = document.getElementById("db-list");
    items.forEach((item) => {
      const itemName = isTeams ? item.id : item.name;
      const itemContent = isTeams ? item.shared_prompt : item.content;
      const el = document.createElement("div");
      el.className = "db-list-item";
      el.dataset.name = itemName;
      el.textContent = itemName;
      el.onclick = () => selectItem(itemName, itemContent);
      list.appendChild(el);
    });
    if (items.length) {
      const first = items[0];
      selectItem(isTeams ? first.id : first.name, isTeams ? first.shared_prompt : first.content);
    }
  } catch (e) {
    document.getElementById("db-list").innerHTML =
      `<div class="muted" style="padding:8px">Failed: ${escapeHtml(e.message)}</div>`;
  }

  document.getElementById("db-save-btn").addEventListener("click", async () => {
    if (!selectedName) return;
    const content = document.getElementById("db-edit-content").value;
    try {
      if (isTeams) {
        await api(`/teams/${encodeURIComponent(selectedName)}`, {
          method: "PATCH",
          body: JSON.stringify({ shared_prompt: content }),
        });
        await loadTeams();
      } else {
        await api(`/prompts/personalities/${encodeURIComponent(selectedName)}`, {
          method: "PUT",
          body: JSON.stringify({ name: selectedName, content }),
        });
      }
      toast(`"${selectedName}" saved.`);
    } catch (e) {
      toast(`Save failed: ${e.message}`, true);
    }
  });

  if (!isTeams) {
    document.getElementById("db-new-btn").addEventListener("click", () => {
      openCreatePersonalityModal(() => openPromptDatabaseModal("personalities"));
    });
  }
}

document.getElementById("btn-prompt-db").onclick = openPromptDatabaseModal;

// ---------------------------------------------------------------------------
// Agent actions (direct tool calls, bypass LLM)
// ---------------------------------------------------------------------------

let _pendingAction = null;

async function executeAction(tool, args = "") {
  const agentId = state.selectedAgentId;
  if (!agentId) return;

  document.querySelectorAll(".action-btn").forEach((b) => (b.disabled = true));

  try {
    const entries = await api(`/agents/${encodeURIComponent(agentId)}/action`, {
      method: "POST",
      body: JSON.stringify({ tool, args }),
    });
    appendConversationEntries(agentId, entries);
  } catch (e) {
    toast(`Action failed: ${e.message}`, true);
  } finally {
    document.querySelectorAll(".action-btn").forEach((b) => (b.disabled = false));
  }
}

document.querySelectorAll(".action-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    const tool = btn.dataset.action;
    const needsArg = btn.dataset.needsArg;

    if (needsArg) {
      _pendingAction = tool;
      document.getElementById("action-arg-label").textContent = needsArg + ":";
      const input = document.getElementById("action-arg-input");
      input.placeholder = needsArg.toLowerCase();
      input.value = "";
      document.getElementById("action-arg-row").classList.remove("hidden");
      input.focus();
    } else {
      executeAction(tool);
    }
  });
});

document.getElementById("action-arg-go").addEventListener("click", () => {
  if (!_pendingAction) return;
  const args = document.getElementById("action-arg-input").value.trim();
  executeAction(_pendingAction, args);
  document.getElementById("action-arg-row").classList.add("hidden");
  _pendingAction = null;
});

document.getElementById("action-arg-cancel").addEventListener("click", () => {
  document.getElementById("action-arg-row").classList.add("hidden");
  _pendingAction = null;
});

document.getElementById("action-arg-input").addEventListener("keydown", (e) => {
  if (e.key === "Enter") document.getElementById("action-arg-go").click();
  if (e.key === "Escape") document.getElementById("action-arg-cancel").click();
});

// ---------------------------------------------------------------------------
// API base configuration
// ---------------------------------------------------------------------------

const apiBaseInput = document.getElementById("api-base");
apiBaseInput.value = state.apiBase;
apiBaseInput.addEventListener("change", () => {
  state.apiBase = apiBaseInput.value.trim().replace(/\/+$/, "");
  localStorage.setItem("zap.apiBase", state.apiBase);
  state.selectedTeamId = null;
  state.selectedAgentId = null;
  state.feedLastSince = Date.now() / 1000 - 60;
  loadTeams();
});

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

applyFeedVisibility();
loadTeams();
setInterval(loadTeams, 15000);

loadStatus();
setInterval(loadStatus, 5000);

loadFeed();
setInterval(loadFeed, 2000);

setInterval(tickRelTimes, 5000);
