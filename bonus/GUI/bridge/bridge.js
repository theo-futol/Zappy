import net from 'node:net';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { WebSocketServer, WebSocket } from 'ws';
import http from 'http';

const TCP_HOST = process.env.ZAPPY_SERVER_HOST || 'localhost';
const TCP_PORT = parseInt(process.env.ZAPPY_SERVER_PORT, 10) || 4242;
const WEB_PORT = parseInt(process.env.GUI_PORT, 10) || 8081;
const TILE_UPDATE_INTERVAL_MS = parseInt(process.env.TILE_UPDATE_INTERVAL_MS, 10) || 5000;

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const BROWSER_DIR = path.join(__dirname, '..', 'browser');
const MIME = {
    '.html': 'text/html',
    '.js':   'application/javascript',
    '.css':  'text/css',
    '.png':  'image/png',
    '.ico':  'image/x-icon',
};

let mapUpdateInterval = null;

let GameState = {
    mapSize: { x: 0, y: 0}, 
    teams: [],
    players: {},
    tiles: {},
    eggs: {},
    timeUnit: 100
};

const server = http.createServer((req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');

    if (req.url === '/api/state' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(GameState));
        return;
    }

    const urlPath = req.url === '/' ? '/index.html' : req.url.split('?')[0];
    const filePath = path.join(BROWSER_DIR, urlPath);
    const ext = path.extname(filePath);
    try {
        const data = fs.readFileSync(filePath);
        res.writeHead(200, { 'Content-Type': MIME[ext] || 'application/octet-stream' });
        res.end(data);
    } catch {
        res.writeHead(404);
        res.end('Not found');
    }
});

const wss = new WebSocketServer({ server });

wss.on('connection', (ws) => {
    console.log('client connected');
    ws.send(JSON.stringify({ type: 'WELCOME', data: GameState }));

    ws.on('close', () => {
        console.log('client disconnected');
    }); 
});

function broadcast(data) {
    const msg = JSON.stringify(data);

    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(msg);
        }
    });
}

const tcpClient = new net.Socket();
let tcpBuffer = '';

tcpClient.connect(TCP_PORT, TCP_HOST, () => {
    console.log(`Connected to zappy server: ${TCP_HOST} ${TCP_PORT}`);
});

tcpClient.on('data', (chunk) => {
    tcpBuffer += chunk.toString();
    let newlineIndex;
    
    while ((newlineIndex = tcpBuffer.indexOf('\n')) !== -1) {
        const line = tcpBuffer.slice(0, newlineIndex).trim();
        tcpBuffer = tcpBuffer.slice(newlineIndex + 1);
        
        if (line) processLine(line);
    }
});

tcpClient.on('close', () => {
    console.log('TCP connection to Zappy server closed.');
    broadcast({ type: 'server_closed', data: {} });
    
    if (mapUpdateInterval) clearInterval(mapUpdateInterval);
    
    wss.close();
    server.close(() => {
        console.log('Web server closed.');
        process.exit(0);
    });
});

tcpClient.on('error', (err) => {
    console.error('TCP Client Error:', err.message);
    process.exit(1);
});

function startMapUpdateLoop() {
    if (mapUpdateInterval) clearInterval(mapUpdateInterval);
    mapUpdateInterval = setInterval(() => {
        if (!tcpClient.destroyed) {
            tcpClient.write('mct\n');
        }
    }, TILE_UPDATE_INTERVAL_MS);
}

function escapeHtml(unsafe) {
    if (!unsafe) return "";
    return String(unsafe)
         .replace(/&/g, "&amp;")
         .replace(/</g, "&lt;")
         .replace(/>/g, "&gt;")
         .replace(/"/g, "&quot;")
         .replace(/'/g, "&#039;");
}

function parseId(idStr) {
    if (idStr.startsWith('#')) return parseInt(idStr.slice(1), 10);
    return parseInt(idStr, 10);
}

// Handlers
function handleMapSize(tokens) {
    if (tokens.length !== 2) return;
    const x = parseInt(tokens[0], 10);
    const y = parseInt(tokens[1], 10);
    if (isNaN(x) || isNaN(y)) return;
    GameState.mapSize = { x, y };
    broadcast({ type: 'msz', data: { x, y } });
}

function handleTileContent(tokens) {
    if (tokens.length !== 9) return;
    const [x, y, q0, q1, q2, q3, q4, q5, q6] = tokens.map(t => parseInt(t, 10));
    if (tokens.some(t => isNaN(parseInt(t, 10)))) return;
    const key = `${x},${y}`;
    const tile = {
        x, y,
        resources: {
            food: q0,
            linemate: q1,
            deraumere: q2,
            sibur: q3,
            mendiane: q4,
            phiras: q5,
            thystame: q6
        }
    };
    GameState.tiles[key] = tile;
    broadcast({ type: 'bct', data: tile });
}

function handleTeamNames(tokens) {
    if (tokens.length < 1) return;
    const teamName = escapeHtml(tokens.join(' '));
    if (!GameState.teams.includes(teamName)) {
        GameState.teams.push(teamName);
    }
    broadcast({ type: 'tna', data: { name: teamName } });
}

function handlePlayerNew(tokens) {
    if (tokens.length < 6) return;
    const id = parseId(tokens[0]);
    const x = parseInt(tokens[1], 10);
    const y = parseInt(tokens[2], 10);
    const o = parseInt(tokens[3], 10);
    const l = parseInt(tokens[4], 10);
    const n = escapeHtml(tokens.slice(5).join(' '));
    
    if (isNaN(id) || isNaN(x) || isNaN(y) || isNaN(o) || isNaN(l)) return;

    const player = { id, x, y, orientation: o, level: l, team: n };
    GameState.players[id] = player;
    broadcast({ type: 'pnw', data: player });
}

function handlePlayerPosition(tokens) {
    if (tokens.length !== 4) return;
    const id = parseId(tokens[0]);
    const x = parseInt(tokens[1], 10);
    const y = parseInt(tokens[2], 10);
    const o = parseInt(tokens[3], 10);
    
    if (isNaN(id) || isNaN(x) || isNaN(y) || isNaN(o)) return;

    if (GameState.players[id]) {
        GameState.players[id].x = x;
        GameState.players[id].y = y;
        GameState.players[id].orientation = o;
    }
    broadcast({ type: 'ppo', data: { id, x, y, orientation: o } });
}

function handlePlayerLevel(tokens) {
    if (tokens.length !== 2) return;
    const id = parseId(tokens[0]);
    const l = parseInt(tokens[1], 10);
    if (isNaN(id) || isNaN(l)) return;

    if (GameState.players[id]) {
        GameState.players[id].level = l;
    }
    broadcast({ type: 'plv', data: { id, level: l } });
}

function handlePlayerStateUpdate(tokens) {
    if (tokens.length !== 12) return;
    const id = parseId(tokens[0]);
    const x = parseInt(tokens[1], 10);
    const y = parseInt(tokens[2], 10);
    const o = parseInt(tokens[3], 10);
    const l = parseInt(tokens[4], 10);
    const [q0, q1, q2, q3, q4, q5, q6] = tokens.slice(5).map(t => parseInt(t, 10));

    if (isNaN(id) || isNaN(x) || isNaN(y) || isNaN(o) || isNaN(l) || 
        isNaN(q0) || isNaN(q1) || isNaN(q2) || isNaN(q3) || isNaN(q4) || isNaN(q5) || isNaN(q6)) return;

    const inventory = {
        food: q0, linemate: q1, deraumere: q2, sibur: q3, mendiane: q4, phiras: q5, thystame: q6
    };
    
    if (GameState.players[id]) {
        GameState.players[id].x = x;
        GameState.players[id].y = y;
        GameState.players[id].orientation = o;
        GameState.players[id].level = l;
        GameState.players[id].inventory = inventory;
    }
    broadcast({ type: 'pipi', data: { id, x, y, orientation: o, level: l, resources: inventory } });
}

function handlePlayerInventory(tokens) {
    if (tokens.length !== 10) return;
    const id = parseId(tokens[0]);
    const x = parseInt(tokens[1], 10);
    const y = parseInt(tokens[2], 10);
    const [q0, q1, q2, q3, q4, q5, q6] = tokens.slice(3).map(t => parseInt(t, 10));

    if (isNaN(id) || isNaN(x) || isNaN(y) || isNaN(q0) || isNaN(q1) || isNaN(q2) || isNaN(q3) || isNaN(q4) || isNaN(q5) || isNaN(q6)) return;

    const inventory = {
        food: q0, linemate: q1, deraumere: q2, sibur: q3, mendiane: q4, phiras: q5, thystame: q6
    };
    if (GameState.players[id]) {
        GameState.players[id].inventory = inventory;
    }
    broadcast({ type: 'pin', data: { id, x, y, resources: inventory } });
}

function handleExpulsion(tokens) {
    if (tokens.length !== 1) return;
    const id = parseId(tokens[0]);
    if (isNaN(id)) return;
    broadcast({ type: 'pex', data: { id } });
}

function handleBroadcast(tokens) {
    if (tokens.length < 2) return;
    const id = parseId(tokens[0]);
    const msg = escapeHtml(tokens.slice(1).join(' '));
    if (isNaN(id)) return;
    broadcast({ type: 'pbc', data: { id, message: msg } });
}

function handleIncantationStart(tokens) {
    if (tokens.length < 4) return;
    const x = parseInt(tokens[0], 10);
    const y = parseInt(tokens[1], 10);
    const l = parseInt(tokens[2], 10);
    const ids = tokens.slice(3).map(t => parseId(t));
    if (isNaN(x) || isNaN(y) || isNaN(l) || ids.some(isNaN)) return;
    broadcast({ type: 'pic', data: { x, y, level: l, players: ids } });
}

function handleIncantationEnd(tokens) {
    if (tokens.length !== 3) return;
    const x = parseInt(tokens[0], 10);
    const y = parseInt(tokens[1], 10);
    const r = escapeHtml(tokens[2]);
    if (isNaN(x) || isNaN(y)) return;
    broadcast({ type: 'pie', data: { x, y, result: r } });
}

function handleEggLaying(tokens) {
    if (tokens.length !== 1) return;
    const id = parseId(tokens[0]);
    if (isNaN(id)) return;
    broadcast({ type: 'pfk', data: { id } });
}

function handleResourceDropping(tokens) {
    if (tokens.length !== 2) return;
    const id = parseId(tokens[0]);
    const i = parseInt(tokens[1], 10);
    if (isNaN(id) || isNaN(i)) return;
    broadcast({ type: 'pdr', data: { id, resource: i } });
}

function handleResourceCollecting(tokens) {
    if (tokens.length !== 2) return;
    const id = parseId(tokens[0]);
    const i = parseInt(tokens[1], 10);
    if (isNaN(id) || isNaN(i)) return;
    broadcast({ type: 'pgt', data: { id, resource: i } });
}

function handlePlayerDeath(tokens) {
    if (tokens.length !== 1) return;
    const id = parseId(tokens[0]);
    if (isNaN(id)) return;
    if (GameState.players[id]) {
        delete GameState.players[id];
    }
    broadcast({ type: 'pdi', data: { id } });
}

function handleEggLaid(tokens) {
    if (tokens.length !== 4) return;
    const e = parseId(tokens[0]);
    const id = parseId(tokens[1]);
    const x = parseInt(tokens[2], 10);
    const y = parseInt(tokens[3], 10);
    if (isNaN(e) || isNaN(id) || isNaN(x) || isNaN(y)) return;
    
    GameState.eggs[e] = { id: e, playerId: id, x, y };
    broadcast({ type: 'enw', data: { eggId: e, playerId: id, x, y } });
}

function handleEggConnection(tokens) {
    if (tokens.length !== 1) return;
    const e = parseId(tokens[0]);
    if (isNaN(e)) return;
    if (GameState.eggs[e]) {
        delete GameState.eggs[e];
    }
    broadcast({ type: 'ebo', data: { eggId: e } });
}

function handleEggDeath(tokens) {
    if (tokens.length !== 1) return;
    const e = parseId(tokens[0]);
    if (isNaN(e)) return;
    if (GameState.eggs[e]) {
        delete GameState.eggs[e];
    }
    broadcast({ type: 'edi', data: { eggId: e } });
}

function handleTimeUnitRequest(tokens) {
    if (tokens.length !== 1) return;
    const t = parseInt(tokens[0], 10);
    if (isNaN(t)) return;
    GameState.timeUnit = t;
    broadcast({ type: 'sgt', data: { timeUnit: t } });
}

function handleTimeUnitModification(tokens) {
    if (tokens.length !== 1) return;
    const t = parseInt(tokens[0], 10);
    if (isNaN(t)) return;
    GameState.timeUnit = t;
    broadcast({ type: 'sst', data: { timeUnit: t } });
}

function handleEndGame(tokens) {
    if (tokens.length < 1) return;
    const n = escapeHtml(tokens.join(' '));
    broadcast({ type: 'seg', data: { teamName: n } });
}

function handleServerMessage(tokens) {
    if (tokens.length < 1) return;
    const msg = escapeHtml(tokens.join(' '));
    broadcast({ type: 'smg', data: { message: msg } });
}

function handleUnknownCommand(tokens) {
    broadcast({ type: 'suc', data: {} });
}

function handleCommandParameter(tokens) {
    broadcast({ type: 'sbp', data: {} });
}

const commandHandlers = {
    'msz': handleMapSize,
    'bct': handleTileContent,
    'tna': handleTeamNames,
    'pnw': handlePlayerNew,
    'ppo': handlePlayerPosition,
    'plv': handlePlayerLevel,
    'pin': handlePlayerInventory,
    'pipi': handlePlayerStateUpdate,
    'pex': handleExpulsion,
    'pbc': handleBroadcast,
    'pic': handleIncantationStart,
    'pie': handleIncantationEnd,
    'pfk': handleEggLaying,
    'pdr': handleResourceDropping,
    'pgt': handleResourceCollecting,
    'pdi': handlePlayerDeath,
    'enw': handleEggLaid,
    'ebo': handleEggConnection,
    'edi': handleEggDeath,
    'sgt': handleTimeUnitRequest,
    'sst': handleTimeUnitModification,
    'seg': handleEndGame,
    'smg': handleServerMessage,
    'suc': handleUnknownCommand,
    'sbp': handleCommandParameter
};

function processLine(line) {
    if (line === 'WELCOME') {
        tcpClient.write('GRAPHIC\n');
        tcpClient.write('msz\n');
        tcpClient.write('tna\n');
        tcpClient.write('sgt\n');
        startMapUpdateLoop();
        return;
    }
    console.log(`server: ${line}`);
    
    const parts = line.trim().split(/\s+/);
    if (parts.length === 0) return;
    const cmd = parts[0];
    const tokens = parts.slice(1);

    if (commandHandlers[cmd]) {
        commandHandlers[cmd](tokens);
    }
}

server.listen(WEB_PORT, () => {
    console.log(`Web API running on port: ${WEB_PORT}`)
});

