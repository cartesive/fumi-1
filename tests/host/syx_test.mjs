// SPDX-License-Identifier: GPL-3.0-only
// web/bench/syx.js: pack / unpack round trips, a bank made here parses back, a bad checksum is refused.
import { parseSyx, makeBank, pack, unpack, initVoice, morph, voiceName, setVoiceName, FP, byteMax } from "../../web/bench/syx.js";

let fail = 0;
const check = (what, ok) => { console.log(`  ${ok ? "ok  " : "FAIL"} ${what}`); if (!ok) fail++; };

// random packed records unpack into range and round-trip through pack
let seed = 7;
const rnd = () => (seed = (seed * 1103515245 + 12345) & 0x7fffffff) & 0x7f;
let okRange = true, okRound = true;
for (let n = 0; n < 200; n++) {
  const pk = new Uint8Array(128);
  for (let i = 0; i < 128; i++) pk[i] = rnd();
  const v = unpack(pk);
  for (let i = 0; i < FP.SIZE; i++) if (v[i] > byteMax(i)) okRange = false;
  const v2 = unpack(pack(v));
  for (let i = 0; i < FP.SIZE; i++) if (v[i] !== v2[i]) okRound = false;
}
check("200 random records: unpack stays in range", okRange);
check("200 random records: pack(unpack) round trips", okRound);

// a bank of 32 voices made here parses back with the same bytes and names
const voices = [];
for (let k = 0; k < 32; k++) {
  const v = initVoice();
  v[FP.ALG] = k;
  v[(6 - 1) * FP.OP + FP.OL] = 99 - k;
  setVoiceName(v, `TEST ${k}`);
  voices.push(pack(v));
}
const bank = makeBank(voices);
check("a 32-voice bank is 4104 bytes", bank.length === 4104);
const parsed = parseSyx(bank, "test");
check("it parses to 32 voices", parsed.length === 32);
check("names and algorithms survive", parsed.every((p, k) => p.name === `TEST ${k}` && p.voice[FP.ALG] === k));
check("bytes survive", parsed.every((p, k) => p.packed.every((b, i) => b === voices[k][i])));

// a corrupted checksum is refused, so is a truncated bank
const bad = new Uint8Array(bank);
bad[6 + 4096] ^= 1;
let threw = "";
try { parseSyx(bad, "bad"); } catch (e) { threw = e.message; }
check("a wrong checksum is refused", /checksum/.test(threw));
threw = "";
try { parseSyx(bank.subarray(0, 3000), "short"); } catch (e) { threw = e.message; }
check("a truncated file is refused", /truncated/.test(threw));

// the morph: the ends are the voices, the middle interpolates levels and keeps discrete values whole
const a = initVoice(), b = initVoice();
b[(6 - 1) * FP.OP + FP.OL] = 49;
b[(6 - 1) * FP.OP + FP.FC] = 7;
setVoiceName(a, "AAAA"); setVoiceName(b, "BBBB");
const m0 = morph(a, b, 0), m5 = morph(a, b, 0.5), m1 = morph(a, b, 1);
check("morph at 0 and 1 are the ends", m0[(6 - 1) * FP.OP + FP.OL] === 99 && m1[(6 - 1) * FP.OP + FP.OL] === 49);
check("morph at 0.5 interpolates the level", m5[(6 - 1) * FP.OP + FP.OL] === 74);
check("morph keeps the coarse ratio whole", m5[(6 - 1) * FP.OP + FP.FC] === 7 || m5[(6 - 1) * FP.OP + FP.FC] === 1);
check("morph names itself", voiceName(m5) === "AAA~BBB50");

console.log(fail ? `syx: ${fail} FAILED` : "syx: all passed");
process.exit(fail ? 1 : 0);
