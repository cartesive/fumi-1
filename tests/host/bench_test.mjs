// SPDX-License-Identifier: GPL-3.0-only
// The bench exports of the browser build in Node: parameters are listed, a patch goes in and comes back,
// a note by pitch sounds, a parameter change reaches the engine, the built-ins are there.
import fs from "fs";
const wasm = fs.readFileSync(process.argv[2]);
let mem;
const wasi = new Proxy({ clock_time_get: (id, p, out) => { new DataView(mem.buffer).setBigUint64(out, 0n, true); return 0; },
  fd_write: (fd, iov, n, out) => { new DataView(mem.buffer).setUint32(out, 0, true); return 0; } }, { get: (t, k) => k in t ? t[k] : () => 0 });
const { instance } = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasi, env: new Proxy({}, { get: () => () => 0 }) });
const ex = instance.exports; mem = ex.memory;
if (ex._initialize) ex._initialize();
let fail = 0;
const check = (what, ok) => { console.log(`  ${ok ? "ok  " : "FAIL"} ${what}`); if (!ok) fail++; };
const str = (ptr) => { const m = new Uint8Array(mem.buffer); let s = ""; for (let i = ptr; m[i] && i < ptr + 32; i++) s += String.fromCharCode(m[i]); return s; };
const render = (frames) => { let pk = 0; for (let i = 0; i < frames; i += 128) { ex.web_render(128); const l = new Float32Array(mem.buffer, ex.web_out_l(), 128); for (const v of l) pk = Math.max(pk, Math.abs(v)); } return pk; };

ex.web_master(3000); ex.web_boot();
check("parameter 0 is Hon", str(ex.web_param_name(0)) === "Hon");
check("Hon ranges 0..15 and starts at 1本 (4)", ex.web_param_lo(0) === 0 && ex.web_param_hi(0) === 15 && ex.web_param_get(0) === 4);
check("built-in patches present", ex.web_npatch() >= 4);
const b0 = new Uint8Array(mem.buffer, ex.web_builtin(0), 128).slice();
render(512);
check("the label is the first built-in", str(ex.web_label()).trim().length > 0 && ex.web_custom() === 0);
ex.web_param(26, 0);                                   // reverb off (P_REVERB)
render(256);
check("silence at rest", render(1024) === 0);
ex.web_note(5, 1, 0);                                   // A4 by pitch
const pk = render(8192);
check("a note by pitch sounds", pk > 0.02 && pk < 1);
check("one voice", ex.web_nvoices() === 1);
ex.web_note(5, 0, 0);
render(44100 * 3);                                      // 余韻 80: about 2.4 s to -60 dB
check("let go: quiet again", ex.web_nvoices() === 0);
// a patch in and back
const p = new Uint8Array(b0);
p[118] = 0x5a; p[119] = 0x5a;                            // name "ZZ"
new Uint8Array(mem.buffer, ex.web_patch(), 128).set(p);
ex.web_patch_set();
render(256);
check("a sent patch is custom and named", ex.web_custom() === 1 && str(ex.web_label()).startsWith("ZZ"));
ex.web_patch_get();
const back = new Uint8Array(mem.buffer, ex.web_patch(), 128);
check("it comes back byte for byte", back.every((v, i) => v === p[i]));
ex.web_key(5, 1);
render(4096);
check("a panel key sounds and lights", ex.web_key_level(5) > 0.05);
ex.web_key(5, 0);
ex.web_panic();
render(256);
check("panic: no voices", ex.web_nvoices() === 0);
console.log(fail ? `bench: ${fail} FAILED` : "bench: all passed");
process.exit(fail ? 1 : 0);
