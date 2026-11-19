// Replays examples/wasm/script.json on the demo module under Node's WASI and writes the last
// frame to standard output, as host_main.cpp does on the host.
//   node replay.mjs demo.wasm script.json > frame.raw
import { WASI } from 'node:wasi';
import { readFile } from 'node:fs/promises';

const script = JSON.parse(await readFile(process.argv[3], 'utf8'));
const wasi = new WASI({ version: 'preview1', args: ['demo'], env: {}, returnOnExit: true });
const mod = await WebAssembly.compile(await readFile(process.argv[2]));
const inst = await WebAssembly.instantiate(mod, wasi.getImportObject());
wasi.initialize(inst);  // _initialize of the reactor
const x = inst.exports;
x.web_init();
let at = 0, px = 0, py = 0, down = 0, capture = 0;
const events = script.events ?? [], captures = script.capture ?? [script.end];
const size = x.web_stride() * x.web_height();
const out = [];
for (let t = 0; t <= script.end; t += script.step) {
  while (at < events.length && events[at][0] <= t) [, px, py, down] = events[at++];
  x.web_step(t, px, py, down);
  // The frame at each capture time, one after the other, as host_main.cpp writes them.
  while (capture < captures.length && captures[capture] <= t) {
    out.push(Buffer.from(Buffer.from(x.memory.buffer, x.web_frame(), size)));
    capture++;
  }
}
process.stdout.write(Buffer.concat(out));
