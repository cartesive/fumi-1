// SPDX-License-Identifier: GPL-3.0-only
// web/bench/session.js: the session file never carries the bytes of a patch that came from someone else's
// .syx (or was derived from one); such patches travel as a fingerprint plus notes and ratings, and get their
// bytes back when the same .syx is loaded again. FuMi's own patches travel whole.
import { fingerprint, serialize, deserialize, reattach, derivedFrom } from "../../web/bench/session.js";

let fail = 0;
const check = (what, ok) => { console.log(`  ${ok ? "ok  " : "FAIL"} ${what}`); if (!ok) fail++; };

const bytes = (seed) => Uint8Array.from({ length: 128 }, (_, i) => (i * 7 + seed) & 0x7f);
const own = { id: "a1", name: "KOTO A", source: "FuMi built-in", packed: bytes(1), rating: 3, notes: { attack: "clicky" }, exp: true, rom: false };
const rom = { id: "b2", name: "KOTO", source: "rom1a.syx", packed: bytes(2), rating: 5, notes: { body: "warm" }, exp: false, rom: true };
const kid = { id: "c3", name: "KOT~HAR50", source: "morph", packed: bytes(3), rating: 1, notes: {}, exp: false, rom: derivedFrom([own, rom]) };

check("a child of a ROM patch is a ROM patch", kid.rom === true);
check("a child of own patches only is not", derivedFrom([own, own]) === false);
check("fingerprints differ by bytes and are short hex", fingerprint(bytes(1)) !== fingerprint(bytes(2)) && /^[0-9a-f]{8,16}$/.test(fingerprint(bytes(1))));

const s = serialize([own, rom, kid]);
const txt = JSON.stringify(s);
check("own patch bytes are in the file", s[0].packed && s[0].packed.length === 128);
check("ROM patch bytes are not in the file, its fingerprint is", !s[1].packed && s[1].fp === fingerprint(bytes(2)));
check("a derived patch's bytes are not in the file either", !s[2].packed && s[2].fp === fingerprint(bytes(3)));
check("the ROM patch's notes and rating are kept", s[1].rating === 5 && s[1].notes.body === "warm");
check("no ROM byte sequence appears anywhere in the file", !txt.includes(JSON.stringify(Array.from(bytes(2)))));

const back = deserialize(s);
check("deserialize keeps own patches playable", back[0].packed && back[0].packed[5] === bytes(1)[5]);
check("ROM patches come back as pending (no bytes)", back[1].packed === null && back[1].pending === true);
const found = reattach(back, [{ name: "KOTO", packed: bytes(2) }, { name: "OTHER", packed: bytes(9) }]);
check("loading the .syx again reattaches the bytes to the pending patch by fingerprint", found === 1 && back[1].packed && back[1].pending === false);
check("the reattached patch keeps its notes", back[1].notes.body === "warm");

console.log(fail ? `session: ${fail} FAILED` : "session: all passed");
process.exit(fail ? 1 : 0);
