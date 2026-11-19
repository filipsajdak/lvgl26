// Drives headless Chrome over the DevTools protocol: opens the page, waits, takes a screenshot,
// clicks with real input events, takes another, and prints what the page reports.
//   node drive-chrome.mjs <page.html> <out-prefix> [chrome] [--expect-title <start>] [--run <seconds>]
// With --expect-title it only loads the page, and exits 0 if the title starts with the text.
import { spawn } from 'node:child_process';
import { writeFile, mkdtemp } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const args = process.argv.slice(2);
const expectAt = args.indexOf('--expect-title');
const expected = expectAt >= 0 ? args.splice(expectAt, 2)[1] : null;
const runAt = args.indexOf('--run');
const runSeconds = runAt >= 0 ? +args.splice(runAt, 2)[1] : 0;
const [page, prefix, chrome = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'] = args;
const port = 9333 + Math.floor(Math.random() * 500);
const profile = await mkdtemp(join(tmpdir(), 'chrome-'));
const proc = spawn(chrome, [...(process.env.HEADED ? [] : ['--headless=new', '--disable-gpu']), `--remote-debugging-port=${port}`, `--user-data-dir=${profile}`,
  '--window-size=700,800', ...(process.env.CI ? ['--no-sandbox'] : []), 'about:blank'], { stdio: 'ignore' });
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
if (expected !== null) {
  const title = await evaluate('document.title');
  await shot('title');
  console.log('title:', title);
  ws.close(); proc.kill();
  process.exit(title.startsWith(expected) ? 0 : 1);
}
if (runSeconds > 0) {
  // No clicks: let the page run, sample its report each ten seconds, and look for a trap.
  await shot('start');
  for (let t = 0; t < runSeconds; t += 10) {
    await sleep(Math.min(10, runSeconds - t) * 1000);
    console.log(`${String(t + 10).padStart(3)} s:`, JSON.stringify(await state()));
  }
  await shot('end');
  const title = await evaluate('document.title');
  ws.close(); proc.kill();
  console.log('title:', title);
  process.exit(title.startsWith('TRAP') ? 1 : 0);
}
await shot('before');
console.log('before:', JSON.stringify(await state()));

const mouse = (type, px, py) => send('Input.dispatchMouseEvent', { type, x: rect.x + px, y: rect.y + py, button: 'left', buttons: type === 'mouseReleased' ? 0 : 1, clickCount: 1 });
async function click(px, py, holdMs = 80) { await mouse('mouseMoved', px, py); await mouse('mousePressed', px, py); await sleep(holdMs); await mouse('mouseReleased', px, py); await sleep(300); }
// The "+" button, three times, with no time between press and release: both events come
// within one frame of the page, and the pointer's queue keeps them for the next step.
await click(293, 225, 0);
await click(293, 225, 0);
await click(293, 225, 0);
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
const states = await evaluate(`[0, 1, 2, 3, 4].map(i => window.demo.x.web_state(i))`);
console.log('states (dial, clicks, ticks, slider, switch):', JSON.stringify(states));
ws.close(); proc.kill();
// Three clicks on "+" make 30, the drag moved the slider, the switch is on, the task ticked.
const [dial, clicks, ticks, slider, lamp] = states;
const ok = dial === 30 && clicks === 3 && ticks > 0 && slider > 0 && lamp === 1;
console.log(ok ? 'interactive check: OK' : 'interactive check: FAILED');
process.exit(ok ? 0 : 1);
