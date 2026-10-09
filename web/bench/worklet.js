// SPDX-License-Identifier: GPL-3.0-only
// FuMi-1's engine in an AudioWorklet for the audition bench: fumi.wasm is the whole FM-1 app (the same build
// as the emulator, web/emu/fumi_web.c) plus the bench exports: the patch, every parameter, notes by pitch.
// Each render quantum asks it for 128 frames, which runs the device's clock forward. About 30 times a
// second the screen, the lights, the levels and the engine's state go out. FoMni's worklet, extended.

const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();

class FumiBench extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.sentBlits = -1;
    this.lastFrame = 0;
    this.busy = 0;
    this.frames = 0;
    this.rmsAcc = 0;
    this.rmsN = 0;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  str(ptr) {
    const m = new Uint8Array(this.mem.buffer);
    let s = "";
    for (let i = ptr; m[i] && i < ptr + 32; i++) s += String.fromCharCode(m[i]);
    return s;
  }

  async onMessage(m) {
    if (m.type === "load") {
      const now = () => BigInt(Math.round(currentTime * 1e9));
      const wasi = {
        clock_time_get: (id, prec, out) => { new DataView(this.mem.buffer).setBigUint64(out, now(), true); return 0; },
        fd_write: (fd, iov, n, out) => { new DataView(this.mem.buffer).setUint32(out, 0, true); return 0; },
        proc_exit: () => {},
      };
      const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
      const env = new Proxy({}, { get: () => () => 0 });
      const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
      this.ex = instance.exports;
      this.mem = this.ex.memory;
      if (this.ex._initialize) this.ex._initialize();
      this.ex.web_master(m.master ?? 3000);
      this.ex.web_boot();
      // the parameter table and the built-in patches, once
      const params = [];
      for (let p = 0; ; p++) {
        const name = this.str(this.ex.web_param_name(p));
        if (p > 0 && name === this.str(this.ex.web_param_name(0))) break;
        params.push({ p, name, lo: this.ex.web_param_lo(p), hi: this.ex.web_param_hi(p), value: this.ex.web_param_get(p) });
        if (p > 80) break;
      }
      const builtins = [];
      for (let i = 0; i < this.ex.web_npatch(); i++)
        builtins.push(new Uint8Array(this.mem.buffer, this.ex.web_builtin(i), 128).slice());
      this.port.postMessage({ type: "ready", params, builtins });
    } else if (!this.ex) {
      return;
    } else if (m.type === "buttons") {
      this.ex.web_buttons(m.mask >>> 0);
    } else if (m.type === "keys") {
      this.ex.web_keys(m.mask >>> 0);
    } else if (m.type === "key") {
      this.ex.web_key(m.key | 0, m.on ? 1 : 0);
    } else if (m.type === "note") {
      this.ex.web_note(m.id | 0, m.on ? 1 : 0, +m.cents || 0);
    } else if (m.type === "ornament") {
      this.ex.web_ornament(m.orn | 0, m.on ? 1 : 0);
    } else if (m.type === "bend") {
      this.ex.web_bend(m.which | 0, m.down ? 1 : 0, m.held ? 1 : 0);
    } else if (m.type === "enc") {
      this.ex.web_enc(m.role, m.n | 0);
    } else if (m.type === "master") {
      this.ex.web_master(m.value | 0);
    } else if (m.type === "param") {
      this.ex.web_param(m.p | 0, m.v | 0);
    } else if (m.type === "patch") {
      new Uint8Array(this.mem.buffer, this.ex.web_patch(), 128).set(m.packed.subarray(0, 128));
      this.ex.web_patch_set();
    } else if (m.type === "panic") {
      this.ex.web_panic();
    }
  }

  publish() {
    const ex = this.ex;
    const blits = ex.web_blits();
    const msg = { type: "frame", buttons: ex.web_lit_buttons() >>> 0, keys: ex.web_lit_keys() >>> 0,
                  peak: ex.web_peak(), nvoices: ex.web_nvoices(), label: this.str(ex.web_label()), custom: ex.web_custom(),
                  bend: ex.web_bend_cents(), rms: this.rmsN ? Math.sqrt(this.rmsAcc / this.rmsN) : 0 };
    this.rmsAcc = 0;
    this.rmsN = 0;
    msg.levels = new Float32Array(27);
    for (let k = 0; k < 27; k++) msg.levels[k] = ex.web_key_level(k);
    msg.params = [];
    for (let p = 0; p < 64; p++) msg.params.push(ex.web_param_get(p));
    if (this.frames >= 44100) {
      msg.load = Math.round(100 * this.busy / (this.frames / 44.1));
      this.busy = this.frames = 0;
    }
    const transfer = [];
    if (blits !== this.sentBlits) {
      this.sentBlits = blits;
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
      transfer.push(msg.fb.buffer);
    }
    this.port.postMessage(msg, transfer);
  }

  process(inputs, outputs) {
    if (!this.ex) return true;
    const out = outputs[0];
    const n = out[0].length;
    const t0 = clock();
    this.ex.web_render(n);
    this.busy += clock() - t0;
    this.frames += n;
    const l = new Float32Array(this.mem.buffer, this.ex.web_out_l(), n);
    out[0].set(l);
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
    for (let i = 0; i < n; i++) this.rmsAcc += l[i] * l[i];
    this.rmsN += n;
    if (currentTime - this.lastFrame >= 1 / 30) {
      this.lastFrame = currentTime;
      this.publish();
    }
    return true;
  }
}

registerProcessor("fumi-bench", FumiBench);
