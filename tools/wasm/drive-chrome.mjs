// Drives headless Chrome over the DevTools protocol: opens the page, waits, takes a screenshot,
// clicks with real input events, takes another, and prints what the page reports.
//   node drive-chrome.mjs <page.html> <out-prefix> [chrome]
import { spawn } from 'node:child_process';
import { writeFile, mkdtemp } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const [page, prefix, chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'] = process.argv.slice(2);
const port = 9333 + Math.floor(Math.random() * 500);
const profile = await mkdtemp(join(tmpdir(), 'chrome-'));
const proc = spawn(chrome, ['--headless=new', '--disable-gpu', `--remote-debugging-port=${port}`, `--user-data-dir=${profile}`,
  '--window-size=700,800', 'about:blank'], { stdio: 'ignore' });
const sleep = ms => new Promise(r => setTimeout(r, ms));

let targets;
for (let i = 0; i < 50 && !targets; i++) {
  try { targets = await (await fetch(`http://127.0.0.1:${port}/json`)).json(); } catch { await sleep(200); }
}
const ws = new WebSocket(targets.find(t => t.type === 'page').webSocketDebuggerUrl);
await new Promise(r => ws.addEventListener('open', r));
let id = 0; const waiting = new Map();
ws.addEventListener('message', e => { const m = JSON.parse(e.data); if (m.id && waiting.has(m.id)) { waiting.get(m.id)(m); waiting.delete(m.id); } });
const send = (method, params = {}) => new Promise(r => { const n = ++id; waiting.set(n, r); ws.send(JSON.stringify({ id: n, method, params })); });
const evaluate = async expr => (await send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true })).result.result.value;
const shot = async name => writeFile(`${prefix}-${name}.png`, Buffer.from((await send('Page.captureScreenshot', { format: 'png' })).result.data, 'base64'));

await send('Page.enable');
await send('Page.navigate', { url: page.startsWith('http') || page.startsWith('file:') ? page : 'file://' + page });
await sleep(1500);
const rect = await evaluate(`(() => { const r = document.getElementById('screen').getBoundingClientRect(); return {x: r.left, y: r.top}; })()`);
const state = () => evaluate(`document.getElementById('stats').textContent`);
await shot('before');
console.log('before:', JSON.stringify(await state()));

const mouse = (type, px, py) => send('Input.dispatchMouseEvent', { type, x: rect.x + px, y: rect.y + py, button: 'left', buttons: type === 'mouseReleased' ? 0 : 1, clickCount: 1 });
async function click(px, py, holdMs = 80) { await mouse('mouseMoved', px, py); await mouse('mousePressed', px, py); await sleep(holdMs); await mouse('mouseReleased', px, py); await sleep(300); }
await click(293, 225);  // the "+" button, three times
await click(293, 225);
await click(293, 225);
// Drag the slider from its left end to the right
await mouse('mouseMoved', 90, 305); await mouse('mousePressed', 90, 305);
for (let px = 100; px <= 230; px += 10) { await mouse('mouseMoved', px, 305); await sleep(16); }
await mouse('mouseReleased', 230, 305); await sleep(300);
await click(317, 304);  // the switch
await sleep(800);
await shot('after');
console.log('after: ', JSON.stringify(await state()));
const frames = await evaluate(`window.demo ? window.demo.stats.frames : -1`);
console.log('frames stepped:', frames);
console.log('latencies (ms):', JSON.stringify(await evaluate(`window.demo.stats.latencies.map(v => +v.toFixed(1))`)));
ws.close(); proc.kill();
