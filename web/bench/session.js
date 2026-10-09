// SPDX-License-Identifier: GPL-3.0-only
// The bench's session file (bench/sessions/*.json goes into the repo). The hard rule: no Yamaha ROM patch
// data in the tree. So a patch that came from someone else's .syx, or was made from one (a morph, a hybrid,
// a variation), travels without its bytes: a fingerprint of the packed record, plus name, source, notes and
// rating. When the same .syx is loaded again the bytes are reattached by fingerprint. FuMi's own patches and
// what is made from them alone travel whole. Pure functions; the DOM is in bench.js.

// FNV-1a over the 128 bytes, 64 bits as hex: enough to identify a record, not a security hash
export function fingerprint(packed) {
  let h = 0xcbf29ce484222325n;
  for (const b of packed) { h ^= BigInt(b & 0xff); h = (h * 0x100000001b3n) & 0xffffffffffffffffn; }
  return h.toString(16).padStart(16, "0");
}

// a patch made from these parents is ROM-derived if any parent is
export function derivedFrom(parents) { return parents.some((p) => !!p.rom); }

// keepRomBytes: only for the browser's own autosave (localStorage), never for a file that goes into the repo
export function serialize(patches, keepRomBytes = false) {
  return patches.map((p) => {
    const o = { id: p.id, name: p.name, source: p.source, rating: p.rating || 0, notes: p.notes || {}, exp: !!p.exp, rom: !!p.rom };
    if (p.packed) o.fp = fingerprint(p.packed);
    if ((!p.rom || keepRomBytes) && p.packed) o.packed = Array.from(p.packed);
    return o;
  });
}

export function deserialize(list) {
  return list.map((o) => ({ id: o.id, name: o.name, source: o.source, rating: o.rating || 0, notes: o.notes || {}, exp: !!o.exp, rom: !!o.rom,
                            fp: o.fp || null, packed: o.packed ? Uint8Array.from(o.packed) : null, pending: !o.packed }));
}

// voices just loaded from a .syx ({name, packed}): give pending patches their bytes back; returns how many
export function reattach(patches, voices) {
  let n = 0;
  const byFp = new Map(voices.map((v) => [fingerprint(v.packed), v.packed]));
  for (const p of patches) {
    if (!p.pending || !p.fp) continue;
    const b = byFp.get(p.fp);
    if (b) { p.packed = new Uint8Array(b); p.pending = false; n++; }
  }
  return n;
}
