#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Analyse a reference recording of the ST-50 (or any instrument) for tuning and timbre.

  tools/analyze_ref.py FILE [--from S] [--to S] [--out REPORT.md] [--a4 440]
                       [--sections] [--peaks] [--min-sil 0.4] [--sil-db -50]
  tools/analyze_ref.py --selftest

This is the proper-tool version of the scratch analyses behind docs/research/13 and 14.
FILE is anything ffmpeg can decode; it is turned into 44.1 kHz mono. The report (markdown,
to stdout or --out) always has the pitch-per-pitch-class table and the scale inferred from
it, plus the per-note timbre figures (attack, click, decay, partials) for every note that
sounds alone for at least 0.45 s. --sections adds the table of silence-separated sections,
--peaks adds the long-FFT spectral-peak table used for a run of ringing notes (file 14).

Pitch is by YIN on 90 ms frames every 20 ms. Offsets are in cents from equal temperament at
--a4. Needs numpy and scipy, and ffmpeg on PATH."""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
from scipy.signal import find_peaks

SR = 44100
NOTE_NAMES = ["C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"]
# The in scale as semitones above mi, lower row then upper row of the ST-50 keyboard.
LOWER_ROW = {0: "mi", 1: "fa", 5: "la", 7: "ti", 8: "do"}
UPPER_ROW = {2: "fa#", 3: "sol", 6: "tib", 9: "do#", 10: "re"}
MIN_NOTE_S = 0.45


# ---------------------------------------------------------------- decoding and envelopes

def decode(path, start=None, end=None):
    """Decode FILE with ffmpeg to 44.1 kHz mono float32, optionally trimmed to [start, end] seconds."""
    cmd = ["ffmpeg", "-v", "error", "-i", str(path)]
    if start is not None:
        cmd += ["-ss", f"{start:.3f}"]
    if end is not None:
        cmd += ["-to", f"{end:.3f}"]
    cmd += ["-f", "f32le", "-ac", "1", "-ar", str(SR), "-"]
    raw = subprocess.run(cmd, check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.float32).astype(np.float64)


def db(x):
    return 20 * np.log10(np.maximum(np.asarray(x, dtype=np.float64), 1e-12))


def rms_envelope(x, win_s, hop_s):
    """RMS over win_s windows every hop_s seconds, as (times, rms). Window starts at each time."""
    win, hop = int(win_s * SR), max(1, int(hop_s * SR))
    cs = np.concatenate([[0.0], np.cumsum(x * x)])
    starts = np.arange(0, max(1, len(x) - win), hop)
    rms = np.sqrt((cs[starts + win] - cs[starts]) / win)
    return starts / SR, rms


def spectrum(x, start_s, win_s, pad=8):
    """Hann-windowed, zero-padded magnitude spectrum of win_s seconds from start_s: (freqs, mag)."""
    i0 = int(start_s * SR)
    seg = x[i0:i0 + int(win_s * SR)]
    if len(seg) < 16:
        return np.array([0.0]), np.array([1e-12])
    seg = seg * np.hanning(len(seg))
    n = 1 << (len(seg) * pad - 1).bit_length()
    mag = np.abs(np.fft.rfft(seg, n))
    return np.fft.rfftfreq(n, 1 / SR), mag


def centroid(x, start_s, win_s=0.05):
    f, mag = spectrum(x, start_s, win_s, pad=1)
    return float(np.sum(f * mag) / max(np.sum(mag), 1e-12))


# ---------------------------------------------------------------- sections and onsets

def find_sections(x, min_sil_s, sil_db):
    """Split at silences longer than min_sil_s below sil_db dBFS. Returns [(start_s, end_s)]."""
    t, env = rms_envelope(x, 0.02, 0.005)
    active = db(env) > sil_db
    if not active.any():
        return []
    edges = np.flatnonzero(np.diff(active.astype(int)))
    # Runs of silence are the gaps between a falling edge and the next rising edge.
    sections, start = [], t[np.argmax(active)]
    falls = [e for e in edges if active[e]]
    for fall in falls:
        rise = next((e for e in edges if e > fall and not active[e]), None)
        gap_end = t[rise + 1] if rise is not None else t[-1] + 0.005
        if gap_end - t[fall + 1] >= min_sil_s:
            sections.append((start, t[fall + 1]))
            start = gap_end
    last_active = t[len(active) - 1 - np.argmax(active[::-1])] + 0.02
    if last_active > start:
        sections.append((start, last_active))
    return [(a, b) for a, b in sections if b - a >= 0.05]


def find_onsets(x, sec, sil_db):
    """Energy-rise onsets in a section: a jump of 12 dB within 20 ms on a 1 ms RMS envelope.

    Returns [(onset_s, peak_s)] with the onset refined to the point 20 dB below the peak."""
    a, b = int(sec[0] * SR), int(sec[1] * SR)
    t, env = rms_envelope(x[a:b], 0.005, 0.001)
    e = db(env)
    rise = np.zeros_like(e)
    rise[20:] = e[20:] - e[:-20]
    cands = np.flatnonzero((rise > 12) & (e > sil_db))
    onsets, last = [], -1000
    for i in cands:
        if i - last < 50:
            last = i
            continue
        last = i
        peak = i + int(np.argmax(e[i:i + 250]))
        below = np.flatnonzero(e[max(0, i - 20):peak] < e[peak] - 20)
        onset = max(0, i - 20) + (below[-1] + 1 if len(below) else 0)
        onsets.append((sec[0] + t[onset], sec[0] + t[peak]))
    return onsets


# ---------------------------------------------------------------- YIN pitch

def yin_frame(frame, tau_min, tau_max, threshold=0.1):
    """One YIN estimate: (period in samples, cmnd at the minimum). Lower cmnd is more periodic."""
    n = len(frame)
    w = n - tau_max
    cs = np.concatenate([[0.0], np.cumsum(frame * frame)])
    size = 1 << (n - 1).bit_length()
    corr = np.fft.irfft(np.fft.rfft(frame, size) * np.conj(np.fft.rfft(frame[:w], size)))[:tau_max + 1]
    taus = np.arange(tau_max + 1)
    d = cs[w] + (cs[taus + w] - cs[taus]) - 2 * corr
    # Cumulative mean normalised difference, with d'(0) = 1.
    cmnd = np.ones_like(d)
    cmnd[1:] = d[1:] * taus[1:] / np.maximum(np.cumsum(d[1:]), 1e-12)
    cmnd[:tau_min] = 1.0
    below = np.flatnonzero(cmnd < threshold)
    if len(below):
        tau = below[0]
        while tau + 1 <= tau_max and cmnd[tau + 1] < cmnd[tau]:
            tau += 1
    else:
        tau = int(np.argmin(cmnd))
    return parabolic_min(cmnd, tau), float(cmnd[tau])


def parabolic_min(y, i):
    """Vertex of the parabola through y[i-1], y[i], y[i+1]; returns i refined (float)."""
    if i <= 0 or i >= len(y) - 1:
        return float(i)
    denom = y[i - 1] - 2 * y[i] + y[i + 1]
    return i + (y[i - 1] - y[i + 1]) / (2 * denom) if denom != 0 else float(i)


def track_pitch(x, sil_db, frame_s=0.09, hop_s=0.02, fmin=60.0, fmax=2000.0):
    """YIN on frame_s frames every hop_s. Returns arrays (time, f0, cmnd, rms_db); f0 is nan when unvoiced."""
    n, hop = int(frame_s * SR), int(hop_s * SR)
    tau_min, tau_max = int(SR / fmax), int(SR / fmin)
    times, f0s, scores, levels = [], [], [], []
    for i0 in range(0, len(x) - n, hop):
        frame = x[i0:i0 + n]
        level = db(np.sqrt(np.mean(frame * frame)))
        if level < sil_db:
            continue
        tau, score = yin_frame(frame, tau_min, tau_max)
        times.append(i0 / SR)
        f0s.append(SR / tau if score < 0.1 else np.nan)
        scores.append(score)
        levels.append(level)
    return (np.array(times), np.array(f0s), np.array(scores), np.array(levels))


def midi_of(f, a4):
    return 69 + 12 * np.log2(f / a4)


def note_name(midi):
    m = int(round(midi))
    return f"{NOTE_NAMES[m % 12]}{m // 12 - 1}"


def cents_offset(f, a4):
    """(nearest MIDI note, offset in cents from it) for frequency f."""
    m = midi_of(f, a4)
    nearest = np.round(m)
    return nearest.astype(int), (m - nearest) * 100


# ---------------------------------------------------------------- tuning tables

def pitch_class_table(f0, a4):
    """Per pitch class: median offset, spread (median absolute deviation) and frame count."""
    f0 = f0[np.isfinite(f0)]
    if len(f0) == 0:
        return {}
    nearest, cents = cents_offset(f0, a4)
    table = {}
    for pc in range(12):
        sel = cents[nearest % 12 == pc]
        if len(sel) < 3:
            continue
        med = float(np.median(sel))
        octaves = sorted(set(int(n // 12 - 1) for n in nearest[nearest % 12 == pc]))
        table[pc] = dict(median=med, spread=float(np.median(np.abs(sel - med))), frames=len(sel), octaves=octaves)
    return table


def infer_scale(table):
    """Find the mi for which the in scale's lower row covers the most frames.

    Returns (mi pitch class, coverage fraction, list of tying candidates)."""
    if not table:
        return None, 0.0, []
    total = sum(v["frames"] for v in table.values())
    scores = {mi: sum(v["frames"] for pc, v in table.items() if (pc - mi) % 12 in LOWER_ROW) for mi in range(12)}
    best = max(scores.values())
    ties = [mi for mi, s in scores.items() if s == best]
    # Among equal coverings prefer a mi that was actually heard, the most often.
    ties.sort(key=lambda mi: -table.get(mi, {}).get("frames", 0))
    return ties[0], best / total, ties


def degree_name(pc, mi):
    step = (pc - mi) % 12
    return LOWER_ROW.get(step) or UPPER_ROW.get(step) or "off"


# ---------------------------------------------------------------- spectral peaks

def spectral_peaks(x, a4, floor_db=50, prominence_db=10, fmin=50, fmax=5000):
    """One long FFT over the selection; peaks with parabolic interpolation, each named by nearest ET note."""
    f, mag = spectrum(x, 0, len(x) / SR, pad=4)
    mag_db = db(mag)
    top = mag_db.max()
    idx, _ = find_peaks(mag_db, height=top - floor_db, prominence=prominence_db)
    idx = idx[(f[idx] >= fmin) & (f[idx] <= fmax)]
    peaks = []
    for i in idx:
        fi = parabolic_min(-mag_db, i) * (f[1] - f[0])
        nearest, cents = cents_offset(np.array([fi]), a4)
        peaks.append(dict(freq=fi, level=float(mag_db[i] - top), note=note_name(nearest[0]), cents=float(cents[0])))
    return mark_partials(sorted(peaks, key=lambda p: p["freq"]))


def mark_partials(peaks):
    """Label a peak as "k x F" when it sits within 15 cents of a multiple of a lower peak."""
    for p in peaks:
        p["partial_of"] = ""
        for q in peaks:
            if q["freq"] >= p["freq"] * 0.9:
                break
            k = round(p["freq"] / q["freq"])
            if 2 <= k <= 8 and abs(1200 * np.log2(p["freq"] / (k * q["freq"]))) < 15:
                p["partial_of"] = f"{k} x {q['freq']:.1f}"
                break
    return peaks


# ---------------------------------------------------------------- per-note timbre

def band_power(seg, fmin):
    """Mean power per sample of seg above fmin Hz."""
    spec = np.fft.rfft(seg)
    spec[np.fft.rfftfreq(len(seg), 1 / SR) < fmin] = 0
    return float(np.mean(np.fft.irfft(spec, len(seg)) ** 2))


def click_db(x, onset_s):
    """Energy above 5 kHz in the first 12 ms after onset relative to the following 100 ms, in dB."""
    i0 = int(onset_s * SR)
    first = x[i0:i0 + int(0.012 * SR)]
    rest = x[i0 + int(0.012 * SR):i0 + int(0.112 * SR)]
    if len(rest) < 100:
        return np.nan
    return float(10 * np.log10(max(band_power(first, 5000), 1e-20) / max(band_power(rest, 5000), 1e-20)))


def decay_rates(x, peak_s, end_s):
    """Slopes in dB/s of the envelope over 0-200 ms and 200-1000 ms after the peak."""
    i0, i1 = int(peak_s * SR), int(end_s * SR)
    t, env = rms_envelope(x[i0:i1], 0.005, 0.001)
    e = db(env)
    rates = []
    for a, b in ((0.0, 0.2), (0.2, 1.0)):
        sel = (t >= a) & (t < b)
        rates.append(float(np.polyfit(t[sel], e[sel], 1)[0]) if sel.sum() > 50 else np.nan)
    return rates


def partial_levels(x, start_s, f0, count=8, win_s=0.06):
    """Levels of partials 1..count in dB relative to the strongest, from a window starting at start_s."""
    f, mag = spectrum(x, start_s, win_s)
    levels = []
    for k in range(1, count + 1):
        sel = (f > k * f0 * 0.97) & (f < k * f0 * 1.03)
        levels.append(db(mag[sel].max()) if sel.any() else -np.inf)
    levels = np.array(levels)
    return levels - levels.max()


def note_pitch(track, start_s, end_s):
    """Median YIN pitch of the voiced frames that lie inside [start_s, end_s]."""
    t, f0 = track[0], track[1]
    sel = (t >= start_s) & (t + 0.09 <= end_s) & np.isfinite(f0)
    return float(np.median(f0[sel])) if sel.sum() >= 3 else np.nan


def analyse_notes(x, sections, track, sil_db, a4):
    """Timbre figures for every note that sounds alone for at least MIN_NOTE_S seconds."""
    notes = []
    for si, sec in enumerate(sections):
        onsets = find_onsets(x, sec, sil_db)
        for j, (onset, peak) in enumerate(onsets):
            end = onsets[j + 1][0] if j + 1 < len(onsets) else sec[1]
            if end - onset < MIN_NOTE_S:
                continue
            f0 = note_pitch(track, onset, end)
            note = dict(section=si, onset=onset, length=end - onset, f0=f0,
                        attack_ms=(peak - onset) * 1000, click=click_db(x, onset),
                        decay=decay_rates(x, peak, end))
            if np.isfinite(f0):
                nearest, cents = cents_offset(np.array([f0]), a4)
                note["name"], note["cents"] = note_name(nearest[0]), float(cents[0])
                note["partials"] = {ms: partial_levels(x, onset + ms / 1000, f0) for ms in (0, 100, 300)}
            notes.append(note)
    return notes


# ---------------------------------------------------------------- report

def fmt(v, digits=1):
    return "n/a" if v is None or not np.isfinite(v) else f"{v:+.{digits}f}" if digits else f"{v:.0f}"


def report_sections(x, sections, t0):
    lines = ["## Sections", "", "| # | Start | End | Length | RMS dB | Centroid at onset | Centroid +300 ms |",
             "|---|---|---|---|---|---|---|"]
    for i, (a, b) in enumerate(sections):
        seg = x[int(a * SR):int(b * SR)]
        level = db(np.sqrt(np.mean(seg * seg)))
        lines.append(f"| {i} | {t0 + a:.2f} | {t0 + b:.2f} | {b - a:.2f} s | {level:.1f} | "
                     f"{centroid(x, a):.0f} Hz | {centroid(x, a + 0.3):.0f} Hz |")
    return lines + [""]


def report_pitch_classes(table, a4):
    lines = [f"## Pitch per pitch class (cents from equal temperament, A4 = {a4:g} Hz)", "",
             "| Note | Offset (cents) | Spread | Frames | Octaves |", "|---|---|---|---|---|"]
    for pc in sorted(table, key=lambda p: -table[p]["frames"]):
        v = table[pc]
        octs = ", ".join(str(o) for o in v["octaves"])
        lines.append(f"| {NOTE_NAMES[pc]} | {v['median']:+.1f} | {v['spread']:.1f} | {v['frames']} | {octs} |")
    return lines + [""]


def report_scale(table):
    mi, coverage, ties = infer_scale(table)
    if mi is None:
        return ["No voiced frames, so no scale.", ""]
    lines = ["### Inferred scale", ""]
    if len(ties) > 1:
        lines.append("Ambiguous: too few pitch classes to fix mi. Candidates for mi: "
                     + ", ".join(NOTE_NAMES[t] for t in ties) + f". Taking {NOTE_NAMES[mi]}.")
    lines.append(f"In scale with mi = {NOTE_NAMES[mi]}: the lower row covers {coverage * 100:.0f}% of voiced frames.")
    mi_offset = table[mi]["median"] if mi in table else 0.0
    if mi not in table:
        lines.append(f"{NOTE_NAMES[mi]} itself was not heard, so the offsets below are from ET, not from mi.")
    lines += ["", "| Degree | Note | Cents from mi | ET | Offset from ET | Frames |", "|---|---|---|---|---|---|"]
    for pc in sorted(table, key=lambda p: (p - mi) % 12):
        step, v = (pc - mi) % 12, table[pc]
        rel = step * 100 + v["median"] - mi_offset
        lines.append(f"| {degree_name(pc, mi)} | {NOTE_NAMES[pc]} | {rel:.1f} | {step * 100} | "
                     f"{v['median'] - mi_offset:+.1f} | {v['frames']} |")
    return lines + ["", "Offsets in this table are relative to mi, so the recording's overall sharpness drops out.", ""]


def report_peaks(peaks):
    lines = ["## Spectral peaks (one FFT over the selection)", "",
             "| Freq (Hz) | Level (dB) | Note | Offset (cents) | Partial of |", "|---|---|---|---|---|"]
    for p in peaks:
        lines.append(f"| {p['freq']:.2f} | {p['level']:.1f} | {p['note']} | {p['cents']:+.1f} | {p['partial_of']} |")
    return lines + [""]


def report_notes(notes, t0):
    lines = [f"## Isolated notes (at least {MIN_NOTE_S} s alone)", ""]
    if not notes:
        return lines + ["None found.", ""]
    lines += ["| Section | Onset | Length | Note | Cents | Attack (ms) | Click (dB) | Decay 0-200 ms (dB/s) | Decay 200-1000 ms (dB/s) |",
              "|---|---|---|---|---|---|---|---|---|"]
    for n in notes:
        lines.append(f"| {n['section']} | {t0 + n['onset']:.2f} | {n['length']:.2f} s | {n.get('name', 'n/a')} | "
                     f"{fmt(n.get('cents'))} | {n['attack_ms']:.1f} | {fmt(n['click'])} | "
                     f"{fmt(n['decay'][0])} | {fmt(n['decay'][1])} |")
    lines += ["", summary_line(notes), "", "### Partials (dB relative to the strongest, window starting at onset + t)", "",
              "| Onset | Note | t (ms) | " + " | ".join(f"P{k}" for k in range(1, 9)) + " |",
              "|---|---|---|" + "---|" * 8]
    for n in notes:
        for ms, levels in n.get("partials", {}).items():
            cells = " | ".join("-inf" if not np.isfinite(v) else f"{v:.0f}" for v in levels)
            lines.append(f"| {t0 + n['onset']:.2f} | {n['name']} | {ms} | {cells} |")
    return lines + [""]


def summary_line(notes):
    def med(key):
        vals = np.array([key(n) for n in notes], dtype=float)
        return fmt(np.median(vals[np.isfinite(vals)])) if np.isfinite(vals).any() else "n/a"
    return (f"Medians over {len(notes)} notes: attack {med(lambda n: n['attack_ms'])} ms, click {med(lambda n: n['click'])} dB, "
            f"decay {med(lambda n: n['decay'][0])} dB/s then {med(lambda n: n['decay'][1])} dB/s.")


def analyse(x, args, title):
    """Run every analysis on the decoded signal and return (markdown lines, notes, pitch table)."""
    t0 = args.start or 0.0
    sections = find_sections(x, args.min_sil, args.sil_db)
    track = track_pitch(x, args.sil_db)
    table = pitch_class_table(track[1], args.a4)
    notes = analyse_notes(x, sections, track, args.sil_db, args.a4)
    lines = [f"# Reference analysis: {title}", "",
             f"Selection {t0:.2f} to {t0 + len(x) / SR:.2f} s ({len(x) / SR:.1f} s), 44.1 kHz mono, "
             f"{len(sections)} sections, {int(np.isfinite(track[1]).sum())} voiced frames.", ""]
    if args.sections:
        lines += report_sections(x, sections, t0)
    lines += report_pitch_classes(table, args.a4) + report_scale(table)
    if args.peaks:
        lines += report_peaks(spectral_peaks(x, args.a4))
    lines += report_notes(notes, t0)
    return lines, notes, table


# ---------------------------------------------------------------- self test

def synth_note(f0, dur_s, partial_db=(0, 0, -2, -9), click=True):
    """A decaying tone: partials at partial_db, 2 ms attack, -28 dB/s for 200 ms then -16 dB/s."""
    t = np.arange(int(dur_s * SR)) / SR
    env_db = np.where(t < 0.2, -28 * t, -28 * 0.2 - 16 * (t - 0.2))
    env = 10 ** (env_db / 20) * np.minimum(1, t / 0.002)
    tone = sum(10 ** (d / 20) * np.sin(2 * np.pi * (k + 1) * f0 * t) for k, d in enumerate(partial_db))
    y = 0.3 * env * tone
    if click:
        n = int(0.010 * SR)
        noise = np.random.default_rng(1).standard_normal(n)
        spec = np.fft.rfft(noise)
        spec[np.fft.rfftfreq(n, 1 / SR) < 6000] = 0
        y[:n] += 0.1 * np.fft.irfft(spec, n) * np.hanning(n)
    return y


def selftest_signal():
    gap = np.zeros(int(0.6 * SR))
    flat = 233.08 * 2 ** (-21 / 1200)
    return np.concatenate([gap, synth_note(220.0, 1.5), gap, synth_note(flat, 1.5), gap])


def selftest(args):
    """Synthesise the test signal, decode it through ffmpeg, analyse, and check the known answers."""
    from scipy.io import wavfile
    with tempfile.TemporaryDirectory() as d:
        path = Path(d) / "selftest.wav"
        wavfile.write(path, SR, selftest_signal().astype(np.float32))
        x = decode(path)
    lines, notes, _ = analyse(x, args, "selftest")
    print("\n".join(lines))
    checks = []
    if len(notes) != 2:
        checks.append((False, f"expected 2 isolated notes, found {len(notes)}"))
        notes = notes + [None] * (2 - len(notes))
    n1, n2 = notes[:2]
    if n1:
        checks += [(abs(1200 * np.log2(n1["f0"] / 220.0)) < 1, f"note 1 pitch {n1['f0']:.3f} Hz, want 220 within 1 cent"),
                   (n1["attack_ms"] < 10, f"note 1 attack {n1['attack_ms']:.1f} ms, want < 10"),
                   (n1["click"] > 5, f"note 1 click {n1['click']:.1f} dB, want > 5"),
                   (-34 < n1["decay"][0] < -22, f"note 1 decay {n1['decay'][0]:.1f} dB/s, want -22 to -34"),
                   (abs(n1["partials"][0][1] - n1["partials"][0][0]) < 2,
                    f"note 1 partial 2 at {n1['partials'][0][1]:.1f} dB vs partial 1 at {n1['partials'][0][0]:.1f}, want within 2")]
    if n2:
        checks.append((n2.get("name") == "Bb3" and abs(n2["cents"] + 21) < 1,
                       f"note 2 is {n2.get('name')} {fmt(n2.get('cents'))} cents, want Bb3 -21 within 1 cent"))
    print("\n## Selftest", "")
    for ok, msg in checks:
        print(("PASS  " if ok else "FAIL  ") + msg)
    failed = [m for ok, m in checks if not ok]
    print(f"\n{len(checks) - len(failed)} of {len(checks)} checks passed.")
    return 0 if not failed else 1


# ---------------------------------------------------------------- main

def parse_args(argv):
    p = argparse.ArgumentParser(description="Tuning and timbre analysis of a reference recording; markdown report.")
    p.add_argument("file", nargs="?", help="audio file (anything ffmpeg decodes)")
    p.add_argument("--from", dest="start", type=float, help="start of the selection, seconds")
    p.add_argument("--to", dest="end", type=float, help="end of the selection, seconds")
    p.add_argument("--out", help="write the markdown report here instead of stdout")
    p.add_argument("--a4", type=float, default=440.0, help="reference pitch for equal temperament (440)")
    p.add_argument("--sections", action="store_true", help="print the table of silence-separated sections")
    p.add_argument("--peaks", action="store_true", help="print the long-FFT spectral-peak table (ringing runs)")
    p.add_argument("--min-sil", type=float, default=0.4, help="silence that separates sections, seconds (0.4)")
    p.add_argument("--sil-db", type=float, default=-50.0, help="silence threshold, dBFS (-50)")
    p.add_argument("--selftest", action="store_true", help="analyse a synthetic signal and check the answers")
    args = p.parse_args(argv)
    if not args.selftest and not args.file:
        p.error("FILE is required unless --selftest")
    return args


def main(argv=None):
    args = parse_args(argv)
    if args.selftest:
        return selftest(args)
    x = decode(args.file, args.start, args.end)
    if len(x) == 0:
        sys.exit("nothing decoded: check the file and the --from/--to range")
    lines, _, _ = analyse(x, args, Path(args.file).name)
    text = "\n".join(lines) + "\n"
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
        print(f"wrote {args.out}")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
