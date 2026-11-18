// SPDX-License-Identifier: GPL-3.0-only
// DX7 SysEx for the audition bench: a 32-voice bank (F0 43 0n 09 20 00, 4096 bytes, checksum, F7) and a
// single voice (F0 43 0n 00 01 1B, 155 bytes, checksum, F7) in; a 32-voice bank out. The 128-byte packed
// record is what the engine takes (fm6_patch.c fm6_unpack); the 155-byte voice is what the sliders edit.
// Checksum: the two's complement of the data's sum, masked to 7 bits. Works in the browser and in Node.

export const FP = {                 // byte offsets inside the 155-byte voice (fm6_core.c FP_*)
  R1: 0, L1: 4, BP: 8, LD: 9, RD: 10, LC: 11, RC: 12, RS: 13, AMS: 14, KVS: 15, OL: 16, MODE: 17, FC: 18, FF: 19, DET: 20,
  OP: 21, PR1: 126, PL1: 130, ALG: 134, FB: 135, OKS: 136, LFS: 137, LFD: 138, LPMD: 139, LAMD: 140, LKS: 141,
  LFW: 142, LPMS: 143, TRNSP: 144, NAME: 145, SIZE: 155,
};
const OPMAX = [99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 3, 3, 7, 3, 7, 99, 1, 31, 99, 14];
const VMAX = [99, 99, 99, 99, 99, 99, 99, 99, 31, 7, 1, 99, 99, 99, 99, 1, 5, 7, 48];
export function byteMax(i) { return i < 126 ? OPMAX[i % 21] : i < FP.NAME ? VMAX[i - 126] : 126; }

export function checksum(data) {
  let s = 0;
  for (const b of data) s += b;
  return (-s) & 0x7f;
}

// the 128-byte record -> the 155-byte voice (every value inside its range; name bytes printable)
export function unpack(b) {
  const v = new Uint8Array(FP.SIZE);
  for (let k = 0; k < 6; k++) {
    const o = k * 17, d = k * FP.OP;
    for (let i = 0; i < 11; i++) v[d + i] = b[o + i] & 0x7f;
    v[d + FP.LC] = b[o + 11] & 3;
    v[d + FP.RC] = (b[o + 11] >> 2) & 3;
    v[d + FP.RS] = b[o + 12] & 7;
    v[d + FP.DET] = (b[o + 12] >> 3) & 15;
    v[d + FP.AMS] = b[o + 13] & 3;
    v[d + FP.KVS] = (b[o + 13] >> 2) & 7;
    v[d + FP.OL] = b[o + 14] & 0x7f;
    v[d + FP.MODE] = b[o + 15] & 1;
    v[d + FP.FC] = (b[o + 15] >> 1) & 31;
    v[d + FP.FF] = b[o + 16] & 0x7f;
  }
  for (let i = 0; i < 9; i++) v[FP.PR1 + i] = b[102 + i] & 0x7f;
  v[FP.ALG] &= 31;
  v[FP.FB] = b[111] & 7;
  v[FP.OKS] = (b[111] >> 3) & 1;
  for (let i = 0; i < 4; i++) v[FP.LFS + i] = b[112 + i] & 0x7f;
  v[FP.LKS] = b[116] & 1;
  v[FP.LFW] = (b[116] >> 1) & 7;
  v[FP.LPMS] = (b[116] >> 4) & 7;
  v[FP.TRNSP] = b[117] & 0x7f;
  for (let i = 0; i < 10; i++) v[FP.NAME + i] = b[118 + i] & 0x7f;
  return sanitize(v);
}

export function sanitize(v) {
  for (let i = 0; i < FP.SIZE; i++) {
    if (i >= FP.NAME) { if (v[i] < 32 || v[i] > 126) v[i] = 32; }
    else if (v[i] > byteMax(i)) v[i] = byteMax(i);
  }
  return v;
}

// the 155-byte voice -> the 128-byte record
export function pack(v) {
  const b = new Uint8Array(128);
  for (let k = 0; k < 6; k++) {
    const o = k * FP.OP, d = k * 17;
    for (let i = 0; i < 11; i++) b[d + i] = v[o + i] & 0x7f;
    b[d + 11] = (v[o + FP.LC] & 3) | (v[o + FP.RC] & 3) << 2;
    b[d + 12] = (v[o + FP.RS] & 7) | (v[o + FP.DET] & 15) << 3;
    b[d + 13] = (v[o + FP.AMS] & 3) | (v[o + FP.KVS] & 7) << 2;
    b[d + 14] = v[o + FP.OL] & 0x7f;
    b[d + 15] = (v[o + FP.MODE] & 1) | (v[o + FP.FC] & 31) << 1;
    b[d + 16] = v[o + FP.FF] & 0x7f;
  }
  for (let i = 0; i < 9; i++) b[102 + i] = v[FP.PR1 + i] & 0x7f;
  b[110] &= 31;
  b[111] = (v[FP.FB] & 7) | (v[FP.OKS] & 1) << 3;
  for (let i = 0; i < 4; i++) b[112 + i] = v[FP.LFS + i] & 0x7f;
  b[116] = (v[FP.LKS] & 1) | (v[FP.LFW] & 7) << 1 | (v[FP.LPMS] & 7) << 4;
  b[117] = v[FP.TRNSP] & 0x7f;
  for (let i = 0; i < 10; i++) b[118 + i] = v[FP.NAME + i] & 0x7f;
  return b;
}

export function voiceName(v) {
  let s = "";
  for (let i = 0; i < 10; i++) s += String.fromCharCode(v[FP.NAME + i]);
  return s.trimEnd();
}
export function setVoiceName(v, name) {
  const s = (name || "").padEnd(10).slice(0, 10);
  for (let i = 0; i < 10; i++) { const c = s.charCodeAt(i); v[FP.NAME + i] = c < 32 || c > 126 ? 32 : c; }
  return v;
}

// every voice in a .syx file: [{name, packed, voice, source}], or throws with a reason. Messages that are not
// DX7 voices are skipped; a wrong checksum or length is an error (nothing half-loaded).
export function parseSyx(bytes, source = "") {
  const out = [];
  let i = 0, n = 0;
  while (i < bytes.length) {
    if (bytes[i] !== 0xf0) { i++; continue; }
    let j = i + 1;
    while (j < bytes.length && bytes[j] !== 0xf7) j++;
    if (j >= bytes.length) throw new Error(`${source}: SysEx message at ${i} has no end (truncated file)`);
    const msg = bytes.subarray(i, j + 1);
    if (msg[1] === 0x43 && (msg[2] & 0xf0) === 0 && msg[3] === 0x09 && msg[4] === 0x20 && msg[5] === 0x00) {
      if (msg.length !== 6 + 4096 + 2) throw new Error(`${source}: 32-voice bank with ${msg.length - 8} data bytes, not 4096`);
      const data = msg.subarray(6, 6 + 4096);
      if (checksum(data) !== msg[6 + 4096]) throw new Error(`${source}: 32-voice bank checksum mismatch`);
      for (let k = 0; k < 32; k++) {
        const packed = new Uint8Array(data.subarray(k * 128, k * 128 + 128));
        const voice = unpack(packed);
        out.push({ name: voiceName(voice), packed: pack(voice), voice, source, index: n++ });
      }
    } else if (msg[1] === 0x43 && (msg[2] & 0xf0) === 0 && msg[3] === 0x00 && msg[4] === 0x01 && msg[5] === 0x1b) {
      if (msg.length !== 6 + 155 + 2) throw new Error(`${source}: single voice with ${msg.length - 8} data bytes, not 155`);
      const data = msg.subarray(6, 6 + 155);
      if (checksum(data) !== msg[6 + 155]) throw new Error(`${source}: single voice checksum mismatch`);
      const voice = sanitize(new Uint8Array(data));
      out.push({ name: voiceName(voice), packed: pack(voice), voice, source, index: n++ });
    }
    i = j + 1;
  }
  return out;
}

// a 32-voice bank from up to 32 packed records (the rest are init voices)
export function makeBank(packedList) {
  const data = new Uint8Array(4096);
  const init = pack(initVoice());
  for (let k = 0; k < 32; k++) data.set(k < packedList.length ? packedList[k] : init, k * 128);
  const out = new Uint8Array(6 + 4096 + 2);
  out.set([0xf0, 0x43, 0x00, 0x09, 0x20, 0x00], 0);
  out.set(data, 6);
  out[6 + 4096] = checksum(data);
  out[6 + 4096 + 1] = 0xf7;
  return out;
}

export function initVoice() {
  const v = new Uint8Array(FP.SIZE);
  for (let k = 0; k < 6; k++) {
    const o = k * FP.OP;
    for (let i = 0; i < 4; i++) { v[o + FP.R1 + i] = 99; v[o + FP.L1 + i] = i < 3 ? 99 : 0; }
    v[o + FP.BP] = 39; v[o + FP.FC] = 1; v[o + FP.DET] = 7; v[o + FP.OL] = k === 5 ? 99 : 0;
  }
  for (let i = 0; i < 4; i++) { v[FP.PR1 + i] = 99; v[FP.PL1 + i] = 50; }
  v[FP.OKS] = 1; v[FP.LFS] = 35; v[FP.LPMS] = 3; v[FP.TRNSP] = 24;
  return setVoiceName(v, "INIT");
}

// a morph between two voices: continuous bytes interpolate, the discrete ones take the nearer side
const DISCRETE_OP = new Set([FP.LC, FP.RC, FP.MODE, FP.FC, FP.DET, FP.AMS, FP.KVS, FP.RS]);
const DISCRETE_V = new Set([FP.ALG, FP.OKS, FP.LKS, FP.LFW, FP.LPMS, FP.TRNSP]);
export function morph(a, b, t) {
  const v = new Uint8Array(FP.SIZE);
  for (let i = 0; i < FP.SIZE; i++) {
    const discrete = i < 126 ? DISCRETE_OP.has(i % FP.OP) : (i >= FP.NAME || DISCRETE_V.has(i));
    v[i] = discrete ? (t < 0.5 ? a[i] : b[i]) : Math.round(a[i] + (b[i] - a[i]) * t);
  }
  return setVoiceName(v, `${voiceName(a).slice(0, 3)}~${voiceName(b).slice(0, 3)}${Math.round(t * 100)}`);
}

// operator k (1..6) of voice v: its 21 bytes start here
export function opOffset(k) { return (6 - k) * FP.OP; }

// the carriers of each algorithm (1..32), as operator numbers
export const CARRIERS = [
  [1, 3], [1, 3], [1, 4], [1, 4], [1, 3, 5], [1, 3, 5], [1, 3], [1, 3], [1, 3], [1, 4], [1, 4], [1, 3], [1, 3], [1, 3],
  [1, 3], [1], [1], [1], [1, 4, 5], [1, 2, 4], [1, 2, 4, 5], [1, 3, 4, 5], [1, 2, 4, 5], [1, 2, 3, 4, 5], [1, 2, 3, 4, 5],
  [1, 2, 4], [1, 2, 4], [1, 3, 6], [1, 2, 3, 5], [1, 2, 3, 6], [1, 2, 3, 4, 5], [1, 2, 3, 4, 5, 6],
];
