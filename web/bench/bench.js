// SPDX-License-Identifier: GPL-3.0-only
// FuMi-1 audition bench (docs/research/16): FuMi's real engine (fumi.wasm, the same build as the browser
// emulator) played from this page, with everything a long voicing session needs: a patch list with instant
// switching, A/B and blind A/B, a morph between two patches, live sliders for every patch byte and for the
// voicing stages, FuMi's key layout on the computer keyboard and on screen, Web MIDI, stock phrases,
// reference clips from refs/ level-matched, notes and ratings per patch, a session file for the repo, and a
// DX7 .syx bank of the finalists for the stock firmware.
import { FP, parseSyx, makeBank, pack, unpack, initVoice, morph, voiceName, setVoiceName, opOffset, byteMax, CARRIERS } from "./syx.js";
import { serialize, deserialize, reattach, derivedFrom } from "./session.js";

"use strict";
const $ = (id) => document.getElementById(id);
const STORE = "fumi-bench-session";
const WHITE_ROW = "qwertyuiasdfghjk", BLACK_ROW = "1234567890-";
const BLACK_ORN = [0, 1, 2, 3, -1, 0, 1, 2, 3, -1, -1];     // ornament mode (ui.c BLACK_ORN): vib, trill, damp, strong
let node = null, ac = null, P = {}, PARAMS = [], ready = false;
const S = { patches: [], cur: -1, a: -1, b: -1, blind: null, history: [], morphBase: null };
const held = new Set();                                      // FuMi key ids down (keyboard + screen)
let octRole = [0, 0];
let logLines = [];

function log(s) {
  const t = new Date().toISOString().slice(11, 19);
  logLines.push(`${t} ${s}`);
  S.history.push({ t: Date.now(), what: s });
  $("log").textContent = logLines.slice(-60).join("\n");
  $("log").scrollTop = 1e9;
  autosave();
}
function send(m) { if (node) node.port.postMessage(m); }
function param(name, v) {
  if (!(name in P)) return;
  send({ type: "param", p: P[name], v: Math.round(v) });
}

// ---------------------------------------------------------------- patches
function newPatch(voice, source, extra = {}) {
  return { id: Math.random().toString(36).slice(2, 9), name: voiceName(voice), voice: new Uint8Array(voice), source,
           rating: 0, notes: { attack: "", body: "", tail: "", gliss: "", registers: "", free: "" }, exp: false, rom: false, pending: false, ...extra };
}
function addPatch(p, quiet) {
  S.patches.push(p);
  if (!quiet) log(`added ${p.name} (${p.source})`);
  renderList();
  return S.patches.length - 1;
}
function current() { return S.cur >= 0 ? S.patches[S.cur] : null; }
let sendTimer = 0;
function sendCurrent(now) {
  const p = current();
  if (!p) return;
  clearTimeout(sendTimer);
  const go = () => send({ type: "patch", packed: pack(p.voice) });
  if (now) go(); else sendTimer = setTimeout(go, 25);
}
function select(i, why) {
  if (i < 0 || i >= S.patches.length) return;
  S.cur = i;
  const p = S.patches[i];
  p.voice = sanitizeInPlace(p.voice);
  sendCurrent(true);
  if (!S.blind) {
    $("curname").textContent = p.name;
    $("curinfo").textContent = `alg ${p.voice[FP.ALG] + 1}, fb ${p.voice[FP.FB]}, from ${p.source}`;
    $("notesname").textContent = p.name;
    $("pname").value = p.name;
  }
  if (!S.blind) {                                            // blind: no editor, notes, marks or row highlight
    $("exportmark").checked = !!p.exp;
    renderEditor();
    renderNotes();
  }
  renderList();
  if (why) log(`playing ${S.blind ? "(blind)" : p.name}${why === "click" ? "" : " " + why}`);
}
function sanitizeInPlace(v) {
  for (let i = 0; i < FP.SIZE; i++) if (i < FP.NAME && v[i] > byteMax(i)) v[i] = byteMax(i);
  return v;
}
function stars(n) { return "★★★★★".slice(0, n) + "☆☆☆☆☆".slice(0, 5 - n); }
function renderList() {
  const box = $("patches");
  box.innerHTML = "";
  S.patches.forEach((p, i) => {
    const d = document.createElement("div");
    d.className = "patch" + (i === S.cur && !S.blind ? " cur" : "") + (p.pending ? " pending" : "");
    const tags = (i === S.a ? '<span class="tag">A</span>' : "") + (i === S.b ? '<span class="tag">B</span>' : "") + (p.exp ? '<span class="tag" style="background:#b36b00">syx</span>' : "");
    d.innerHTML = `<span class="name ${S.blind ? "blind" : ""}">${i < 10 ? `<kbd>${(i + 1) % 10}</kbd> ` : ""}${esc(p.name)}${tags}</span>
      <span class="stars" title="rating">${stars(p.rating)}</span>
      <span class="src">${esc(p.source)} <button data-a="a" title="make this A">A</button> <button data-a="b" title="make this B">B</button> <button data-a="del" title="remove">×</button></span>`;
    d.addEventListener("click", (e) => {
      const a = e.target.dataset.a;
      if (a === "a") { S.a = i; $("aname").textContent = p.name; renderList(); log(`A = ${p.name}`); morphUpdate(); return; }
      if (a === "b") { S.b = i; $("bname").textContent = p.name; renderList(); log(`B = ${p.name}`); morphUpdate(); return; }
      if (a === "del") { if (!confirm(`Remove ${p.name} and its notes?`)) return; S.patches.splice(i, 1); if (S.a === i) S.a = -1; if (S.b === i) S.b = -1; if (S.a > i) S.a--; if (S.b > i) S.b--;
                         if (S.cur === i) S.cur = -1; else if (S.cur > i) S.cur--; log(`removed ${p.name}`); renderList(); return; }
      if (p.pending) { log(`${p.name} has no bytes here: load ${p.source} again (refs/ or Load .syx)`); return; }
      select(i, "click");
    });
    box.appendChild(d);
  });
}
function esc(s) { return String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c])); }

// the patch editor: every byte of the current voice on a slider, live
const OPROWS = [["R1", FP.R1], ["R2", FP.R1 + 1], ["R3", FP.R1 + 2], ["R4", FP.R1 + 3], ["L1", FP.L1], ["L2", FP.L1 + 1], ["L3", FP.L1 + 2], ["L4", FP.L1 + 3],
                ["OL", FP.OL], ["FC", FP.FC], ["FF", FP.FF], ["DET", FP.DET], ["MODE", FP.MODE], ["KVS", FP.KVS], ["RS", FP.RS], ["AMS", FP.AMS],
                ["BP", FP.BP], ["LD", FP.LD], ["LC", FP.LC], ["RD", FP.RD], ["RC", FP.RC]];
const VROWS = [["ALG", FP.ALG], ["FB", FP.FB], ["OKS", FP.OKS], ["TRNSP", FP.TRNSP], ["LFS", FP.LFS], ["LFD", FP.LFD], ["LPMD", FP.LPMD], ["LAMD", FP.LAMD],
               ["LKS", FP.LKS], ["LFW", FP.LFW], ["LPMS", FP.LPMS], ["PR1", FP.PR1], ["PR2", FP.PR1 + 1], ["PR3", FP.PR1 + 2], ["PR4", FP.PR1 + 3],
               ["PL1", FP.PL1], ["PL2", FP.PL1 + 1], ["PL3", FP.PL1 + 2], ["PL4", FP.PL1 + 3]];
function renderEditor() {
  const p = current();
  const ops = $("ops"), vo = $("voice");
  ops.innerHTML = ""; vo.innerHTML = "";
  if (!p) return;
  const car = new Set(CARRIERS[p.voice[FP.ALG] & 31]);
  for (let k = 1; k <= 6; k++) {
    const o = opOffset(k), d = document.createElement("div");
    d.className = "op";
    d.innerHTML = `<h3>OP${k} ${car.has(k) ? '<span class="car">carrier</span>' : '<span class="hint">mod</span>'}</h3>`;
    for (const [name, off] of OPROWS) d.appendChild(slider(name, o + off, byteMax(o + off), p));
    ops.appendChild(d);
  }
  for (const [name, off] of VROWS) vo.appendChild(slider(name, off, byteMax(off), p));
}
function slider(name, idx, max, p) {
  const l = document.createElement("label");
  l.innerHTML = `<span>${name}</span><input type="range" min="0" max="${max}" value="${p.voice[idx]}"><span>${showByte(idx, p.voice[idx])}</span>`;
  const inp = l.querySelector("input"), out = l.querySelector("span:last-child");
  inp.addEventListener("input", () => {
    p.voice[idx] = +inp.value;
    out.textContent = showByte(idx, p.voice[idx]);
    if (idx === FP.ALG) renderEditor();
    sendCurrent(false);
    autosave();
  });
  return l;
}
function showByte(idx, v) {
  if (idx < 126) {
    const r = idx % FP.OP;
    if (r === FP.DET) return (v - 7 > 0 ? "+" : "") + (v - 7);
    if (r === FP.FC) return v === 0 ? "0.5" : String(v);
    if (r === FP.MODE) return v ? "fix" : "rat";
  }
  if (idx === FP.ALG) return String(v + 1);
  if (idx === FP.TRNSP) return (v - 24 > 0 ? "+" : "") + (v - 24);
  return String(v);
}

// ---------------------------------------------------------------- hybrids, variations, morph
function hybrids() {
  if (S.a < 0 || S.b < 0) { alert("Mark a patch as A and one as B first."); return; }
  const A = S.patches[S.a].voice, B = S.patches[S.b].voice, an = voiceName(A).slice(0, 4), bn = voiceName(B).slice(0, 4);
  const src = `hybrid of ${voiceName(A)} + ${voiceName(B)}`, rom = derivedFrom([S.patches[S.a], S.patches[S.b]]);
  for (const t of [0.25, 0.5, 0.75]) addPatch(newPatch(morph(A, B, t), src, { rom }), true);
  const mix = (att, body, name) => {                         // stack A (OP1, OP2) from one, the rest and the voice block from the other
    const v = new Uint8Array(body);
    for (const k of [1, 2]) v.set(att.subarray(opOffset(k), opOffset(k) + FP.OP), opOffset(k));
    v[FP.ALG] = 4;                                           // algorithm 5: three pairs, so the stacks stay separate
    return setVoiceName(v, name);
  };
  addPatch(newPatch(mix(A, B, `${an}>${bn}`), src + " (A's attack stack on B)", { rom }), true);
  addPatch(newPatch(mix(B, A, `${bn}>${an}`), src + " (B's attack stack on A)", { rom }), true);
  const env = new Uint8Array(B);                              // B with A's envelopes
  for (let k = 1; k <= 6; k++) for (let i = 0; i < 8; i++) env[opOffset(k) + i] = A[opOffset(k) + i];
  addPatch(newPatch(setVoiceName(env, `${bn} ${an}env`), src + " (B with A's envelopes)", { rom }), true);
  const full = new Uint8Array(A);                             // A with a quiet octave below on OP5
  full[opOffset(5) + FP.OL] = Math.max(full[opOffset(5) + FP.OL], 60);
  full[opOffset(5) + FP.FC] = 0;
  full[opOffset(5) + FP.MODE] = 0;
  addPatch(newPatch(setVoiceName(full, `${an} +oct`), src + " (A with an octave below)", { rom }), true);
  log(`made 7 hybrids from ${voiceName(A)} and ${voiceName(B)}`);
  renderList();
}
let rnd = Date.now() & 0xffff;
function rand() { rnd = (rnd * 1103515245 + 12345) & 0x7fffffff; return rnd / 0x7fffffff; }
function duplicateVary() {
  const p = current();
  if (!p) return;
  const amt = +$("varyamt").value, v = new Uint8Array(p.voice);
  for (let k = 1; k <= 6; k++) {
    const o = opOffset(k);
    for (let i = 0; i < 8; i++) v[o + i] = clampByte(o + i, v[o + i] + Math.round((rand() * 2 - 1) * amt));
    v[o + FP.OL] = clampByte(o + FP.OL, v[o + FP.OL] + Math.round((rand() * 2 - 1) * amt * 0.6));
    if (rand() < 0.3) v[o + FP.DET] = clampByte(o + FP.DET, v[o + FP.DET] + (rand() < 0.5 ? -1 : 1));
    if (rand() < 0.15 && v[o + FP.FF]) v[o + FP.FF] = clampByte(o + FP.FF, v[o + FP.FF] + Math.round((rand() * 2 - 1) * amt));
  }
  if (rand() < 0.3) v[FP.FB] = clampByte(FP.FB, v[FP.FB] + (rand() < 0.5 ? -1 : 1));
  setVoiceName(v, (p.name.replace(/\+*$/, "") + "+").slice(0, 10));
  const i = addPatch(newPatch(v, `varied from ${p.name} by ${amt}`, { rom: p.rom }));
  select(i, "varied");
}
function clampByte(idx, v) { return Math.max(0, Math.min(byteMax(idx), v)); }
function morphUpdate() {
  if (S.a < 0 || S.b < 0) return;
  const t = +$("morph").value / 100;
  S.morphBase = morph(S.patches[S.a].voice, S.patches[S.b].voice, t);
  send({ type: "patch", packed: pack(S.morphBase) });
  if (!S.blind) $("curname").textContent = `morph ${Math.round(t * 100)}% (A → B)`;
}
function morphKeep() {
  if (!S.morphBase) return;
  const i = addPatch(newPatch(S.morphBase, `morph of ${S.patches[S.a].name} → ${S.patches[S.b].name} at ${$("morph").value}%`, { rom: derivedFrom([S.patches[S.a], S.patches[S.b]]) }));
  select(i, "kept");
}

// ---------------------------------------------------------------- A/B
function abToggle() {
  if (S.a < 0 || S.b < 0) return;
  if (S.blind) { S.blind.which = S.blind.which === "A" ? "B" : "A"; select(S.blind.which === "A" ? S.a : S.b); $("blindwhich").textContent = S.blind.shown === S.blind.which ? "X" : "Y"; return; }
  select(S.cur === S.a ? S.b : S.a, "A/B");
}
function blindStart() {
  if (S.a < 0 || S.b < 0) { alert("Mark a patch as A and one as B first."); return; }
  S.blind = { which: rand() < 0.5 ? "A" : "B", shown: null };
  S.blind.shown = S.blind.which;                             // X is whichever came first
  $("blindbox").hidden = false;
  $("blindwhich").textContent = "X";
  $("curname").textContent = "(blind)"; $("curinfo").textContent = ""; $("notesname").textContent = "(blind)";
  document.querySelectorAll(".patch .name").forEach((e) => e.classList.add("blind"));
  select(S.blind.which === "A" ? S.a : S.b);
  log("blind A/B started");
}
function blindVote() {
  if (!S.blind) return;
  const w = S.blind.which, p = S.patches[w === "A" ? S.a : S.b];
  p.rating = Math.min(5, p.rating + 1);
  log(`blind vote: ${w} (${p.name}) preferred`);
  blindReveal();
}
function blindReveal() {
  if (!S.blind) return;
  const w = S.blind.which;
  S.blind = null;
  $("blindbox").hidden = true;
  log(`revealed: it was ${w}`);
  select(w === "A" ? S.a : S.b);
}

// ---------------------------------------------------------------- notes and ratings
function renderNotes() {
  const p = current();
  const r = $("rating");
  r.innerHTML = "";
  for (let i = 1; i <= 5; i++) {
    const b = document.createElement("button");
    b.textContent = p && p.rating >= i ? "★" : "☆";
    b.addEventListener("click", () => { if (p) { p.rating = p.rating === i ? 0 : i; renderNotes(); renderList(); autosave(); } });
    r.appendChild(b);
  }
  document.querySelectorAll(".prompt input").forEach((inp) => { inp.value = p ? p.notes[inp.dataset.k] || "" : ""; inp.disabled = !p; });
  $("notes").value = p ? p.notes.free || "" : "";
}
document.querySelectorAll(".prompt input").forEach((inp) => inp.addEventListener("input", () => { const p = current(); if (p) { p.notes[inp.dataset.k] = inp.value; autosave(); } }));
$("notes").addEventListener("input", () => { const p = current(); if (p) { p.notes.free = $("notes").value; autosave(); } });
$("pname").addEventListener("change", () => { const p = current(); if (p) { p.name = $("pname").value.slice(0, 10); setVoiceName(p.voice, p.name); sendCurrent(true); renderList(); autosave(); } });
$("exportmark").addEventListener("change", () => { const p = current(); if (p) { p.exp = $("exportmark").checked; renderList(); autosave(); } });

// ---------------------------------------------------------------- session: autosave, export, import, .syx
let saveTimer = 0;
function autosave() { clearTimeout(saveTimer); saveTimer = setTimeout(() => { try { localStorage.setItem(STORE, JSON.stringify(sessionObject(true))); } catch {} }, 300); }
function sessionObject(local) {                               // local: the browser's own copy keeps every byte
  // patches from someone else's .syx (and anything made from them) go without their bytes: a fingerprint,
  // the name, the notes and the rating (session.js). The file goes into the repo; ROM data must not.
  return { app: "FuMi-1 bench", version: 2, date: new Date().toISOString(), a: S.a, b: S.b, cur: S.cur,
           patches: serialize(S.patches.map((p) => ({ ...p, packed: p.pending ? p.packed : pack(p.voice) })), !!local),
           params: Object.fromEntries(PARAMS.map((x) => [x.name, x.value])), history: S.history.slice(-500) };
}
function fromSerialized(list, sourceTag) {
  return deserialize(list).map((d) => {
    const voice = d.packed ? unpack(d.packed) : initVoice();
    const p = newPatch(voice, d.source || "session", { id: d.id, name: d.name, rating: d.rating, notes: d.notes, exp: d.exp, rom: d.rom, pending: d.pending, fp: d.fp });
    if (sourceTag) p.source = `${p.source} (${sourceTag})`;
    p.notes = { attack: "", body: "", tail: "", gliss: "", registers: "", free: "", ...p.notes };
    return p;
  });
}
function loadSession(o, merge) {
  if (!o || !Array.isArray(o.patches)) throw new Error("not a bench session");
  const list = fromSerialized(o.patches, merge ? "imported" : "");
  if (merge) {                                               // import adds; it never throws the current work away
    const have = new Set(S.patches.map((p) => p.id));
    for (const p of list) if (!have.has(p.id)) S.patches.push(p);
  } else {
    S.patches = list;
    S.a = o.a ?? -1; S.b = o.b ?? -1;
    S.history = o.history || [];
  }
  $("aname").textContent = S.a >= 0 ? S.patches[S.a].name : "—";
  $("bname").textContent = S.b >= 0 ? S.patches[S.b].name : "—";
  renderList();
  if (!merge && o.cur >= 0 && o.cur < S.patches.length && !S.patches[o.cur].pending) select(o.cur);
}
// voices just loaded from a .syx: pending patches (from an earlier session) get their bytes back
function reattachVoices(voices) {
  const n = reattach(S.patches, voices.map((v) => ({ name: v.name, packed: v.packed })));
  if (n) { S.patches.forEach((p) => { if (!p.pending && p.packed && p.fp) p.voice = unpack(p.packed); }); log(`${n} patches from the last session got their bytes back`); }
}
function download(name, bytes, type) {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([bytes], { type }));
  a.download = name;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 5000);
}
async function exportSession() {
  const o = sessionObject(), name = `session-${o.date.slice(0, 19).replace(/[:T]/g, "-")}.json`, txt = JSON.stringify(o, null, 1);
  download(name, txt, "application/json");
  try {
    const r = await fetch(`/bench/sessions/${name}`, { method: "POST", body: txt, headers: { "Content-Type": "application/json" } });
    log(r.ok ? `session saved to bench/sessions/${name} (and downloaded)` : `session downloaded (the server did not keep it: ${r.status})`);
  } catch { log("session downloaded (no local server: run web/bench/serve.py to keep it in the repo)"); }
}
function exportSyx() {
  const list = S.patches.filter((p) => p.exp && !p.pending);
  if (!list.length) { alert("Tick 'export to .syx' on the patches to take to the FM-1 (up to 32)."); return; }
  download("fumi-finalists.syx", makeBank(list.slice(0, 32).map((p) => pack(p.voice))), "application/octet-stream");
  log(`exported ${Math.min(32, list.length)} patches as a 32-voice bank`);
}
async function loadSyxFiles(files) {
  for (const f of files) {
    try {
      const voices = parseSyx(new Uint8Array(await f.arrayBuffer()), f.name);
      reattachVoices(voices);
      voices.forEach((v) => addPatch(newPatch(v.voice, f.name, { rom: true }), true));
      log(`loaded ${voices.length} voices from ${f.name}`);
    } catch (e) { log(`refused ${f.name}: ${e.message}`); alert(e.message); }
  }
  renderList();
  autoAB();
}
async function loadRefs() {
  try {
    const r = await fetch("/refs/list");
    if (!r.ok) throw new Error(String(r.status));
    const list = await r.json();
    for (const name of list.syx) {
      const b = new Uint8Array(await (await fetch(`/refs/${encodeURIComponent(name)}`)).arrayBuffer());
      try { const vs = parseSyx(b, name); reattachVoices(vs); vs.forEach((v) => addPatch(newPatch(v.voice, name, { rom: true }), true)); log(`loaded ${name}`); }
      catch (e) { log(`refused ${name}: ${e.message}`); }
    }
    renderClips(list.audio);
    renderList();
    autoAB();
    if (!list.syx.length) log("refs/ holds no .syx (put the DX7 bank with KOTO and HARP 2 there)");
  } catch { log("no local server: run web/bench/serve.py from the repo to load refs/ (or use Load .syx)"); }
}
function autoAB() {                                         // KOTO and HARP 2, if they arrived: A, B and the first hybrids
  if (S.a >= 0 || S.b >= 0) return;
  const k = S.patches.findIndex((p) => /^KOTO\b/i.test(p.name) && !/^KOTO [AB]|WARM/.test(p.name));
  const h = S.patches.findIndex((p) => /^HARP ?2/i.test(p.name));
  if (k >= 0 && h >= 0) {
    S.a = k; S.b = h;
    $("aname").textContent = S.patches[k].name; $("bname").textContent = S.patches[h].name;
    log("KOTO and HARP 2 found: A and B, and a first set of hybrids");
    hybrids();
    select(k, "to start");
  }
}

// ---------------------------------------------------------------- keys, bend, ornaments
const keyEls = [];
function buildKeys() {
  const box = $("keys");
  const W = 16, pitch = 100 / W;
  for (let w = 0; w < W; w++) {
    const d = document.createElement("div");
    d.className = "key white" + (w % 5 === 0 ? " tonic" : "");
    Object.assign(d.style, { left: `${w * pitch + 0.3}%`, width: `${pitch - 0.6}%`, top: "40%", height: "58%" });
    d.textContent = ["mi", "fa", "la", "ti", "do"][w % 5] + (w % 5 === 0 ? ` ${["水", "", "三", "八", "·"][Math.min(4, w / 5 | 0)] || ""}` : "");
    d.dataset.key = w;
    box.appendChild(d);
    keyEls[w] = d;
  }
  const after = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14];
  after.forEach((a, b) => {
    const d = document.createElement("div");
    d.className = "key black";
    Object.assign(d.style, { left: `${(a + 1) * pitch - pitch * 0.3}%`, width: `${pitch * 0.6}%`, top: "2%", height: "36%" });
    d.textContent = ["fa#", "sol", "tib", "do#", "re"][b % 5];
    d.dataset.key = 16 + b;
    box.appendChild(d);
    keyEls[16 + b] = d;
  });
  const down = (e) => { const k = +e.target.dataset.key; if (!Number.isInteger(k)) return; e.preventDefault(); e.target.setPointerCapture?.(e.pointerId); keyOn(k, "p" + e.pointerId); };
  const up = (e) => { for (const id of [...held]) if (String(id).endsWith("p" + e.pointerId)) keyOff(+String(id).split(":")[0], "p" + e.pointerId); };
  box.addEventListener("pointerdown", down);
  box.addEventListener("pointerup", up);
  box.addEventListener("pointercancel", up);
  box.addEventListener("pointermove", (e) => {               // a glissando: the finger moves across the whites
    const src = "p" + e.pointerId;
    const cur = [...held].find((id) => String(id).endsWith(src));
    if (!cur) return;
    for (let w = 0; w < 16; w++) {
      const r = keyEls[w].getBoundingClientRect();
      if (e.clientX >= r.left && e.clientX < r.right) {
        if (+String(cur).split(":")[0] !== w) { keyOff(+String(cur).split(":")[0], src); keyOn(w, src); }
        return;
      }
    }
  });
}
function anyHeld() { return held.size > 0; }
function keyOn(k, src) {
  const id = `${k}:${src}`;
  if (held.has(id)) return;
  held.add(id);
  if (k >= 16 && paramGet("Black keys")) { const o = BLACK_ORN[k - 16]; if (o >= 0) send({ type: "ornament", orn: o, on: 1 }); }
  else send({ type: "key", key: k, on: 1 });
}
function keyOff(k, src) {
  held.delete(`${k}:${src}`);
  if ([...held].some((id) => +String(id).split(":")[0] === k)) return;
  if (k >= 16 && paramGet("Black keys")) { const o = BLACK_ORN[k - 16]; if (o >= 0) send({ type: "ornament", orn: o, on: 0 }); }
  else send({ type: "key", key: k, on: 0 });
}
function octPress(which) {
  if (octRole[1 - which]) { octRole[which] = 2; send({ type: "bend", which, down: 1, held: 0 }); return; }
  if (anyHeld()) { octRole[which] = 1; send({ type: "bend", which, down: 1, held: 1 }); return; }
  octRole[which] = 2;
  param("Octave", paramGet("Octave") + (which ? 1 : -1));
}
function octRelease(which) {
  if (octRole[which] === 1) send({ type: "bend", which, down: 0, held: anyHeld() ? 1 : 0 });
  octRole[which] = 0;
}
function paramGet(name) { const x = PARAMS.find((p) => p.name === name); return x ? x.value : 0; }

addEventListener("keydown", (e) => {
  if (!ready || e.metaKey || e.ctrlKey || e.altKey || e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA") return;
  if (e.repeat) return;
  const c = e.code;
  if (e.shiftKey && /^Digit\d$/.test(c)) { const n = +c.slice(5); select(n === 0 ? 9 : n - 1, "key"); e.preventDefault(); return; }
  if (c === "Backquote") { abToggle(); e.preventDefault(); return; }
  if (c === "Escape") { send({ type: "panic" }); return; }
  if (c === "KeyZ") { octPress(0); return; }
  if (c === "KeyX") { octPress(1); return; }
  const w = WHITE_ROW.indexOf(e.key.toLowerCase());
  if (w >= 0 && /^Key/.test(c)) { keyOn(w, "k"); e.preventDefault(); return; }
  const b = BLACK_ROW.indexOf(e.key);
  if (b >= 0) { keyOn(16 + b, "k"); e.preventDefault(); }
});
addEventListener("keyup", (e) => {
  if (e.code === "KeyZ") { octRelease(0); return; }
  if (e.code === "KeyX") { octRelease(1); return; }
  const w = WHITE_ROW.indexOf(e.key.toLowerCase());
  if (w >= 0 && /^Key/.test(e.code)) keyOff(w, "k");
  const b = BLACK_ROW.indexOf(e.key);
  if (b >= 0) keyOff(16 + b, "k");
});
addEventListener("blur", () => { for (const id of [...held]) keyOff(+String(id).split(":")[0], String(id).split(":")[1]); });
$("octdn").addEventListener("pointerdown", () => octPress(0)); $("octdn").addEventListener("pointerup", () => octRelease(0));
$("octup").addEventListener("pointerdown", () => octPress(1)); $("octup").addEventListener("pointerup", () => octRelease(1));

// ---------------------------------------------------------------- phrases
let phraseTimers = [];
function phraseStop() { phraseTimers.forEach(clearTimeout); phraseTimers = []; for (let k = 0; k < 27; k++) keyOff(k, "ph"); if (octRole[1]) octRelease(1); }
function phrase(name) {
  phraseStop();
  const ev = [];
  const note = (t, k, len) => { ev.push([t, () => keyOn(k, "ph")]); ev.push([t + len, () => keyOff(k, "ph")]); };
  if (name === "registers") { note(0, 0, 1400); note(1600, 5, 1400); note(3200, 10, 1400); note(4800, 15, 1400); }
  if (name === "slow") { [5, 6, 7, 8, 7, 6, 5, 4, 3].forEach((k, i) => note(i * 700, k, 1600)); }
  if (name === "gliss") { for (let w = 0; w < 16; w++) note(w * 60, w, 90); for (let w = 15; w >= 0; w--) note(1600 + (15 - w) * 60, w, 90); }
  if (name === "bend") { note(0, 5, 2200); ev.push([400, () => octPress(1)]); ev.push([1400, () => octRelease(1)]); note(2600, 7, 1200); ev.push([2700, () => octPress(1)]); ev.push([2800, () => octRelease(1)]); }
  if (name === "trill") { const was = paramGet("Trill"); ev.push([0, () => param("Trill", 1)]); note(50, 5, 2400); ev.push([2600, () => param("Trill", was)]); }
  for (const [t, f] of ev) phraseTimers.push(setTimeout(f, t));
}
document.querySelectorAll(".phrase").forEach((b) => b.addEventListener("click", () => phrase(b.dataset.p)));
$("phrasestop").addEventListener("click", phraseStop);

// ---------------------------------------------------------------- reference clips
let clipSrc = null, clipGain = null, clipRms = 0, engineRms = 0.02;
function renderClips(names) {
  const box = $("clips");
  box.innerHTML = "";
  if (!names.length) { box.innerHTML = '<p class="hint">No audio in refs/. Put the ST-50 recordings there (they stay out of git).</p>'; return; }
  for (const n of names) {
    const d = document.createElement("div");
    d.className = "clip";
    d.innerHTML = `<span title="${esc(n)}">${esc(n.length > 28 ? n.slice(0, 26) + "…" : n)}</span><input type="number" step="0.1" min="0" placeholder="from s"><input type="number" step="0.1" min="0" placeholder="to s"><button>Play</button>`;
    const [from, to] = d.querySelectorAll("input");
    d.querySelector("button").addEventListener("click", () => playClip(n, +from.value || 0, +to.value || 0));
    box.appendChild(d);
  }
}
const clipCache = new Map();
async function playClip(name, from, to) {
  clipStop();
  if (!ac) { alert("Switch the bench on first."); return; }
  let buf = clipCache.get(name);
  if (!buf) {
    $("status").textContent = `decoding ${name}…`;
    buf = await ac.decodeAudioData(await (await fetch(`/refs/${encodeURIComponent(name)}`)).arrayBuffer());
    clipCache.set(name, buf);
    $("status").textContent = "On.";
  }
  const a = Math.max(0, Math.min(buf.duration, from)), b = to > a ? Math.min(buf.duration, to) : buf.duration;
  // level matching: the clip's RMS over the region is brought to the bench's own RMS target (-20 dBFS at
  // full clip level), the same target the out meter's mark shows for the engine
  const ch = buf.getChannelData(0), i0 = Math.floor(a * buf.sampleRate), i1 = Math.floor(b * buf.sampleRate);
  let acc = 0;
  for (let i = i0; i < i1; i++) acc += ch[i] * ch[i];
  clipRms = Math.sqrt(acc / Math.max(1, i1 - i0)) || 1e-4;
  clipGain = ac.createGain();
  clipGain.gain.value = clipGainValue();
  clipSrc = ac.createBufferSource();
  clipSrc.buffer = buf;
  clipSrc.connect(clipGain).connect(ac.destination);
  clipSrc.start(0, a, b - a);
  log(`clip ${name} ${a.toFixed(1)}–${b.toFixed(1)} s, gain ${(20 * Math.log10(clipGain.gain.value)).toFixed(1)} dB`);
}
// the clip's RMS is brought to the engine's RMS over its recent playing (a running average of the frames
// that had sound), the slider an offset of -20 .. +20 dB around that: the same loudness, by ear's measure
function clipGainValue() {
  const offset = Math.pow(10, ((+$("cliplevel").value - 50) / 50 * 20) / 20);
  return Math.min(16, (engineRms / (clipRms || 1e-4)) * offset);
}
function clipStop() { try { clipSrc?.stop(); } catch {} clipSrc = null; }
$("clipstop").addEventListener("click", clipStop);
$("cliplevel").addEventListener("input", () => { if (clipGain) clipGain.gain.value = clipGainValue(); });

// ---------------------------------------------------------------- MIDI
async function midiSetup() {
  if (!navigator.requestMIDIAccess) return;
  try {
    const m = await navigator.requestMIDIAccess({ sysex: false });
    const sel = $("midiin");
    const fill = () => { sel.innerHTML = '<option value="">(none)</option>'; for (const i of m.inputs.values()) sel.innerHTML += `<option value="${i.id}">${esc(i.name)}</option>`; };
    fill();
    m.onstatechange = fill;
    let cur = null;
    sel.addEventListener("change", () => {
      if (cur) cur.onmidimessage = null;
      cur = m.inputs.get(sel.value) || null;
      if (cur) cur.onmidimessage = (e) => {
        const [st, n, v] = e.data, s = st & 0xf0;
        if (s === 0x90 && v) send({ type: "note", id: n, on: 1, cents: (n - 69) * 100 });
        else if (s === 0x80 || (s === 0x90 && !v)) send({ type: "note", id: n, on: 0, cents: 0 });
        else if (s === 0xb0 && (n === 123 || n === 120)) send({ type: "panic" });
      };
      log(cur ? `MIDI in: ${cur.name}` : "MIDI in: none");
    });
  } catch {}
}

// ---------------------------------------------------------------- the engine
const ctx = $("screen").getContext("2d"), img = ctx.createImageData(240, 240);
function drawScreen(fb) {
  const d = img.data;
  for (let i = 0; i < 240 * 240; i++) {
    const v = fb[i], p = ((v >> 8) | (v << 8)) & 0xffff;
    d[i * 4] = ((p >> 11) & 31) * 255 / 31; d[i * 4 + 1] = ((p >> 5) & 63) * 255 / 63; d[i * 4 + 2] = (p & 31) * 255 / 31; d[i * 4 + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}
function onFrame(m) {
  if (m.fb) drawScreen(m.fb);
  for (let k = 0; k < 27; k++) keyEls[k]?.classList.toggle("lit", m.levels[k] > 0.12);
  const db = 20 * Math.log10(m.peak || 1e-5);
  $("meter").style.width = `${Math.max(0, Math.min(100, (db + 60) / 60 * 100))}%`;
  $("meter").classList.toggle("hot", db > -3);
  $("bendv").textContent = Math.abs(m.bend) > 0.5 ? `bend ${m.bend > 0 ? "+" : ""}${m.bend.toFixed(0)} c` : "";
  if (m.rms > 0.002) engineRms += (m.rms - engineRms) * 0.05;   // the engine's loudness while it plays (clips follow it)
  if (m.load !== undefined) $("cpu").textContent = `${m.load}% of this computer, ${m.nvoices} voices`;
  PARAMS.forEach((p) => { p.value = m.params[p.p]; });
  syncControls();
}
const CTL = [["Hon", "p_hon", "select"], ["Tuning", "p_tuning", "select"], ["Depth", "p_depth", "range"], ["Scale", "p_scale", "select"], ["Yoin", "p_yoin", "range"],
             ["Vibrato", "p_vib", "check"], ["Vib rate", "p_vibrate", "range"], ["Vib depth", "p_vibdepth", "range"], ["Trill", "p_trill", "check"], ["Trill rate", "p_trillrate", "range"],
             ["Trill var", "p_trillvar", "range"], ["Mono", "p_mono", "check"], ["Slide", "p_slide", "check"], ["Bend up", "p_bendup", "select"], ["Black keys", "p_black", "check"],
             ["Low cut", "p_lowcut", "range"], ["High cut", "p_highcut", "range"], ["Character", "p_character", "range"], ["Reverb", "p_reverb", "range"], ["Rev size", "p_revsize", "range"]];
let syncing = false;
function syncControls() {
  syncing = true;
  for (const [name, id, kind] of CTL) {
    const el = $(id), v = paramGet(name);
    if (document.activeElement === el) continue;
    if (kind === "check") el.checked = !!v; else el.value = v;
  }
  $("v_depth").textContent = `${paramGet("Depth")}%`;
  $("v_yoin").textContent = String(paramGet("Yoin"));
  syncing = false;
}
function wireControls() {
  const HON = ["水4", "水3", "水2", "水1", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"];
  $("p_hon").innerHTML = HON.map((h, i) => `<option value="${i}">${h}本</option>`).join("");
  $("p_tuning").innerHTML = ["平均律 (equal)", "純正律 SUIKO (measured)", "KOTO (pure fifths)", "USER"].map((h, i) => `<option value="${i}">${h}</option>`).join("");
  $("p_scale").innerHTML = ["IN 陰", "YO 陽", "MIN'YO 民謡"].map((h, i) => `<option value="${i}">${h}</option>`).join("");
  for (const [name, id, kind] of CTL) {
    $(id).addEventListener(kind === "range" ? "input" : "change", () => { if (!syncing) param(name, kind === "check" ? ($(id).checked ? 1 : 0) : +$(id).value); });
  }
  $("benchlevel").addEventListener("input", () => param("Level", +$("benchlevel").value));
  $("panic").addEventListener("click", () => { send({ type: "panic" }); phraseStop(); clipStop(); });
}

async function switchOn() {
  const go = $("go");
  go.disabled = true;
  go.textContent = "Starting…";
  try {
    ac = new AudioContext({ sampleRate: 44100, latencyHint: "interactive" });
    const [wasm] = await Promise.all([fetch("fumi.wasm").then((r) => { if (!r.ok) throw new Error(`fumi.wasm: ${r.status}`); return r.arrayBuffer(); }), ac.audioWorklet.addModule("worklet.js")]);
    node = new AudioWorkletNode(ac, "fumi-bench", { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
    node.connect(ac.destination);
    const readyP = new Promise((res) => {
      node.port.onmessage = (e) => {
        const m = e.data;
        if (m.type === "ready") { PARAMS = m.params; P = Object.fromEntries(PARAMS.map((p) => [p.name, p.p])); m.builtins.forEach((b) => addPatch(newPatch(unpack(b), "FuMi built-in"), true)); res(); }
        else if (m.type === "frame") onFrame(m);
      };
    });
    node.port.postMessage({ type: "load", wasm, master: 3000 }, [wasm]);
    await readyP;
    await ac.resume();
    ready = true;
    go.hidden = true;
    $("status").textContent = ac.sampleRate === 44100 ? "On." : `On (the browser runs audio at ${ac.sampleRate} Hz).`;
    param("Level", +$("benchlevel").value);
    param("Reverb", 10);
    const saved = (() => { try { return JSON.parse(localStorage.getItem(STORE) || "null"); } catch { return null; } })();
    if (saved && saved.patches && saved.patches.length) {
      const builtins = S.patches.splice(0);
      try { loadSession(saved); log(`restored the last session (${S.patches.length} patches)`); }
      catch { S.patches = builtins; }
      for (const b of builtins) if (!S.patches.some((p) => p.source === b.source && p.name === b.name)) S.patches.push(b);
    }
    renderList();
    if (S.cur < 0 && S.patches.length) select(0, "to start");
    await loadRefs();
    midiSetup();
  } catch (err) {
    go.disabled = false;
    go.textContent = "Switch on";
    $("status").textContent = "Could not start: " + (err && err.message || err);
  }
}

buildKeys();
wireControls();
renderNotes();
$("go").addEventListener("click", switchOn);
$("loadsyx").addEventListener("click", () => $("syxfile").click());
$("syxfile").addEventListener("change", (e) => loadSyxFiles([...e.target.files]));
$("refsload").addEventListener("click", loadRefs);
$("hybrids").addEventListener("click", hybrids);
$("dup").addEventListener("click", duplicateVary);
$("savecur").addEventListener("click", () => { const p = current(); if (p) { const i = addPatch(newPatch(p.voice, `kept from ${p.name}`, { rom: p.rom })); select(i, "kept"); } });
$("exportjson").addEventListener("click", exportSession);
$("importjson").addEventListener("click", () => $("jsonfile").click());
$("jsonfile").addEventListener("change", async (e) => { try { loadSession(JSON.parse(await e.target.files[0].text()), true); log("session imported (added to this one)"); autosave(); } catch (x) { alert(x.message); } });
$("exportsyx").addEventListener("click", exportSyx);
$("abtoggle").addEventListener("click", abToggle);
$("blind").addEventListener("click", blindStart);
$("blindswap").addEventListener("click", abToggle);
$("votea").addEventListener("click", blindVote);
$("reveal").addEventListener("click", blindReveal);
$("morph").addEventListener("input", morphUpdate);
$("morphkeep").addEventListener("click", morphKeep);
