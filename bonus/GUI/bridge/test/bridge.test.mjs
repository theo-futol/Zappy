// Functional tests for the GUI bridge: a fake Zappy TCP server feeds GUI
// protocol lines to a real bridge process, and we assert the aggregated
// GameState through the bridge's own /api/state endpoint.
//
// Run with: node --test test/

import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import net from 'node:net';
import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const BRIDGE = path.join(__dirname, '..', 'bridge.js');

let tcpServer;
let bridgeProc;
let guiSocket = null;
let tcpPort;
let webPort;

function listen(server) {
    return new Promise((resolve) => server.listen(0, '127.0.0.1', () => resolve(server.address().port)));
}

function push(line) {
    assert.ok(guiSocket, 'bridge is not connected to the fake server');
    guiSocket.write(line + '\n');
}

async function state() {
    const res = await fetch(`http://127.0.0.1:${webPort}/api/state`);
    assert.equal(res.status, 200);
    return res.json();
}

// Poll /api/state until predicate(gameState) is truthy (pushes are async).
// Connection errors are retried: the bridge may still be starting up.
async function waitForState(predicate, what, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;
    let last;
    while (Date.now() < deadline) {
        try {
            last = await state();
            if (predicate(last)) return last;
        } catch (_) { /* bridge not listening yet */ }
        await new Promise((r) => setTimeout(r, 40));
    }
    assert.fail(`timed out waiting for ${what}; last state: ${JSON.stringify(last)}`);
}

before(async () => {
    tcpServer = net.createServer((socket) => {
        guiSocket = socket;
        socket.setNoDelay(true);
        socket.write('WELCOME\n');
        let buf = '';
        socket.on('data', (chunk) => {
            buf += chunk.toString();
            let idx;
            while ((idx = buf.indexOf('\n')) !== -1) {
                const line = buf.slice(0, idx).trim();
                buf = buf.slice(idx + 1);
                // Answer the bridge's initial queries
                if (line === 'msz') socket.write('msz 10 10\n');
                if (line === 'tna') socket.write('tna REDS\ntna BLUES\n');
                if (line === 'sgt') socket.write('sgt 100\n');
            }
        });
        socket.on('error', () => {});
    });
    tcpPort = await listen(tcpServer);

    webPort = 20000 + Math.floor(Math.random() * 10000);
    bridgeProc = spawn(process.execPath, [BRIDGE], {
        env: {
            ...process.env,
            ZAPPY_SERVER_HOST: '127.0.0.1',
            ZAPPY_SERVER_PORT: String(tcpPort),
            GUI_PORT: String(webPort),
            TILE_UPDATE_INTERVAL_MS: '60000',
        },
        stdio: ['ignore', 'ignore', 'inherit'],
    });

    // wait for the bridge to be connected + serving
    await waitForState(() => guiSocket !== null, 'bridge TCP connection');
    push('');
    await waitForState((s) => s.mapSize.x === 10, 'initial msz handling');
});

after(() => {
    bridgeProc?.kill();
    tcpServer?.close();
});

test('handshake: map size, teams and time unit are aggregated', async () => {
    const s = await waitForState((s) => s.teams.length === 2, 'team names');
    assert.deepEqual(s.mapSize, { x: 10, y: 10 });
    assert.deepEqual(s.teams, ['REDS', 'BLUES']);
    assert.equal(s.timeUnit, 100);
});

test('pnw + pipi: player spawns and carries level and inventory', async () => {
    push('pnw #1 2 3 1 1 REDS');
    push('pipi #1 2 3 1 1 9 1 0 0 0 0 0');
    const s = await waitForState((st) => st.players['1']?.inventory, 'player 1 inventory');
    const p = s.players['1'];
    assert.equal(p.team, 'REDS');
    assert.equal(p.level, 1);
    assert.deepEqual(p.inventory, {
        food: 9, linemate: 1, deraumere: 0, sibur: 0, mendiane: 0, phiras: 0, thystame: 0,
    });
});

test('plv: level updates are stored', async () => {
    push('plv #1 3');
    const s = await waitForState((st) => st.players['1']?.level === 3, 'level update');
    assert.equal(s.players['1'].level, 3);
});

test('pin: inventory-only updates are stored', async () => {
    push('pin #1 2 3 5 0 0 0 0 0 1');
    const s = await waitForState((st) => st.players['1']?.inventory.thystame === 1, 'pin update');
    assert.equal(s.players['1'].inventory.food, 5);
});

test('ppo: position/orientation updates are stored', async () => {
    push('ppo #1 4 5 2');
    const s = await waitForState((st) => st.players['1']?.x === 4, 'position update');
    assert.equal(s.players['1'].y, 5);
    assert.equal(s.players['1'].orientation, 2);
});

test('bct: tile resources are aggregated', async () => {
    push('bct 4 5 1 2 0 0 0 0 7');
    const s = await waitForState((st) => st.tiles['4,5'], 'tile update');
    assert.deepEqual(s.tiles['4,5'].resources, {
        food: 1, linemate: 2, deraumere: 0, sibur: 0, mendiane: 0, phiras: 0, thystame: 7,
    });
});

test('enw/ebo: eggs are tracked and removed', async () => {
    push('enw #42 #1 6 6');
    let s = await waitForState((st) => st.eggs['42'], 'egg laid');
    assert.equal(s.eggs['42'].x, 6);
    assert.equal(s.eggs['42'].playerId, 1);

    push('ebo #42');
    s = await waitForState((st) => !st.eggs['42'], 'egg hatched');
});

test('pbc: broadcasts are recorded for conversation history', async () => {
    push('pbc #1 hello_from_one');
    push('pbc #2 answer_from_two');
    const s = await waitForState((st) => (st.broadcasts || []).length >= 2, 'broadcast history');
    const lastTwo = s.broadcasts.slice(-2);
    assert.equal(lastTwo[0].id, 1);
    assert.equal(lastTwo[0].message, 'hello_from_one');
    assert.equal(lastTwo[1].id, 2);
    assert.ok(lastTwo[1].timestamp > 0);
});

test('pdi: dead players are removed from state', async () => {
    push('pnw #2 0 0 1 1 BLUES');
    await waitForState((st) => st.players['2'], 'player 2 spawn');
    push('pdi #2');
    await waitForState((st) => !st.players['2'], 'player 2 removal');
});

test('team names are HTML-escaped before broadcast', async () => {
    push('tna <script>alert(1)</script>');
    const s = await waitForState((st) => st.teams.length === 3, 'escaped team');
    assert.ok(s.teams[2].includes('&lt;script&gt;'));
    assert.ok(!s.teams[2].includes('<script>'));
});

test('browser page is served at /', async () => {
    const res = await fetch(`http://127.0.0.1:${webPort}/`);
    assert.equal(res.status, 200);
    const html = await res.text();
    assert.ok(html.includes('Zappy'));
});

test('malformed lines are ignored without crashing', async () => {
    push('pipi #1 not numbers at all x y z a b c d e');
    push('bct too few');
    push('totallyunknown 1 2 3');
    const s = await waitForState((st) => st.players['1'], 'state still healthy');
    assert.equal(s.players['1'].inventory.food, 5); // unchanged from pin test
});
