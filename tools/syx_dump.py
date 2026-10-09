#!/usr/bin/env python3
"""Dump and diff DX7 patches stored in SysEx files.

Understands the two standard Yamaha DX7 messages:

  32-voice bank  F0 43 0n 09 20 00  + 4096 data bytes + checksum + F7
  single voice   F0 43 0n 00 01 1B  +  155 data bytes + checksum + F7

The bank stores each voice packed into 128 bytes. The single-voice message
uses the 155-byte unpacked layout, one byte per parameter. pack() and
unpack() convert between the two.

Usage examples:

  syx_dump.py BANK.syx                  list the voices
  syx_dump.py BANK.syx --voice 5        show voice 5 in full
  syx_dump.py --diff A.syx:3 B.syx:7    show what differs between two voices
  syx_dump.py BANK.syx --json           dump every voice as JSON
  syx_dump.py --selftest                run the built-in checks
"""

import argparse
import contextlib
import io
import json
import os
import random
import sys
import tempfile

PACKED_SIZE = 128
UNPACKED_SIZE = 155
BANK_DATA_SIZE = 32 * PACKED_SIZE

# Parameter names in the unpacked layout. Each operator block is 21 bytes,
# stored OP6 first. The voice block follows the six operators.
OP_PARAMS = [
    "R1", "R2", "R3", "R4", "L1", "L2", "L3", "L4",
    "BP", "LD", "RD", "LC", "RC", "RS", "AMS", "KVS",
    "OL", "MODE", "FC", "FF", "DET",
]
VOICE_PARAMS = [
    "PR1", "PR2", "PR3", "PR4", "PL1", "PL2", "PL3", "PL4",
    "ALG", "FB", "OKS", "LFS", "LFD", "LPMD", "LAMD",
    "LKS", "LFW", "LPMS", "TRNSP",
]
OP_BLOCK = len(OP_PARAMS)           # 21
VOICE_BLOCK_START = 6 * OP_BLOCK    # 126
NAME_START = VOICE_BLOCK_START + len(VOICE_PARAMS)  # 145

# Maximum legal value of each parameter. Used by the selftest to build
# voices that exercise every bit field without overflowing it.
OP_MAX = {
    "R1": 99, "R2": 99, "R3": 99, "R4": 99,
    "L1": 99, "L2": 99, "L3": 99, "L4": 99,
    "BP": 99, "LD": 99, "RD": 99, "LC": 3, "RC": 3, "RS": 7,
    "AMS": 3, "KVS": 7, "OL": 99, "MODE": 1, "FC": 31, "FF": 99, "DET": 14,
}
VOICE_MAX = {
    "PR1": 99, "PR2": 99, "PR3": 99, "PR4": 99,
    "PL1": 99, "PL2": 99, "PL3": 99, "PL4": 99,
    "ALG": 31, "FB": 7, "OKS": 1, "LFS": 99, "LFD": 99, "LPMD": 99,
    "LAMD": 99, "LKS": 1, "LFW": 5, "LPMS": 7, "TRNSP": 48,
}

# Which operators (1-based) are carriers in each of the 32 algorithms.
# Index 0 is algorithm 1.
CARRIERS = [
    (1, 3), (1, 3), (1, 4), (1, 4), (1, 3, 5), (1, 3, 5), (1, 3), (1, 3),
    (1, 3), (1, 4), (1, 4), (1, 3), (1, 3), (1, 3), (1, 3), (1,),
    (1,), (1,), (1, 4, 5), (1, 2, 4), (1, 2, 4, 5), (1, 3, 4, 5),
    (1, 2, 4, 5), (1, 2, 3, 4, 5), (1, 2, 3, 4, 5), (1, 2, 4), (1, 2, 4),
    (1, 3, 6), (1, 2, 3, 5), (1, 2, 3, 6), (1, 2, 3, 4, 5),
    (1, 2, 3, 4, 5, 6),
]

LFO_WAVES = ["TRIANGLE", "SAW DOWN", "SAW UP", "SQUARE", "SINE", "S&HOLD"]
CURVES = ["-LIN", "-EXP", "+EXP", "+LIN"]
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


# ---------------------------------------------------------------------------
# Packing and unpacking
# ---------------------------------------------------------------------------

def unpack(packed):
    """Expand a 128-byte packed voice into the 155-byte parameter list."""
    if len(packed) != PACKED_SIZE:
        raise ValueError("packed voice must be %d bytes" % PACKED_SIZE)
    out = []
    for op in range(6):
        p = packed[op * 17:(op + 1) * 17]
        out.extend(p[0:11])                     # rates, levels, BP, LD, RD
        out.append(p[11] & 3)                   # LC
        out.append((p[11] >> 2) & 3)            # RC
        out.append(p[12] & 7)                   # RS
        out.append(p[13] & 3)                   # AMS
        out.append((p[13] >> 2) & 7)            # KVS
        out.append(p[14])                       # OL
        out.append(p[15] & 1)                   # MODE
        out.append((p[15] >> 1) & 31)           # FC
        out.append(p[16])                       # FF
        out.append((p[12] >> 3) & 15)           # DET
    out.extend(packed[102:110])                 # PR1-4, PL1-4
    out.append(packed[110])                     # ALG
    out.append(packed[111] & 7)                 # FB
    out.append((packed[111] >> 3) & 1)          # OKS
    out.extend(packed[112:116])                 # LFS LFD LPMD LAMD
    out.append(packed[116] & 1)                 # LKS
    out.append((packed[116] >> 1) & 7)          # LFW
    out.append((packed[116] >> 4) & 7)          # LPMS
    out.append(packed[117])                     # TRNSP
    out.extend(packed[118:128])                 # NAME
    return list(out)


def pack(params):
    """Squeeze a 155-byte parameter list into the 128-byte packed form."""
    if len(params) != UNPACKED_SIZE:
        raise ValueError("unpacked voice must be %d bytes" % UNPACKED_SIZE)
    out = bytearray()
    for op in range(6):
        p = params[op * OP_BLOCK:(op + 1) * OP_BLOCK]
        out.extend(p[0:11])
        out.append((p[11] & 3) | ((p[12] & 3) << 2))        # LC | RC
        out.append((p[13] & 7) | ((p[20] & 15) << 3))       # RS | DET
        out.append((p[14] & 3) | ((p[15] & 7) << 2))        # AMS | KVS
        out.append(p[16])                                   # OL
        out.append((p[17] & 1) | ((p[18] & 31) << 1))       # MODE | FC
        out.append(p[19])                                   # FF
    v = params[VOICE_BLOCK_START:NAME_START]
    out.extend(v[0:8])                                      # PR, PL
    out.append(v[8])                                        # ALG
    out.append((v[9] & 7) | ((v[10] & 1) << 3))             # FB | OKS
    out.extend(v[11:15])                                    # LFO
    out.append((v[15] & 1) | ((v[16] & 7) << 1) | ((v[17] & 7) << 4))
    out.append(v[18])                                       # TRNSP
    out.extend(params[NAME_START:NAME_START + 10])
    return bytes(out)


# ---------------------------------------------------------------------------
# Voice accessors
# ---------------------------------------------------------------------------

def voice_name(params):
    raw = bytes(b & 0x7F for b in params[NAME_START:NAME_START + 10])
    return "".join(chr(b) if 32 <= b < 127 else "?" for b in raw)


def voice_param(params, name):
    return params[VOICE_BLOCK_START + VOICE_PARAMS.index(name)]


def op_param(params, op_number, name):
    """Read one parameter of operator op_number (1-based, OP1 is the first)."""
    slot = 6 - op_number
    return params[slot * OP_BLOCK + OP_PARAMS.index(name)]


def op_values(params, op_number):
    slot = 6 - op_number
    return params[slot * OP_BLOCK:(slot + 1) * OP_BLOCK]


def algorithm(params):
    """Return the 1-based algorithm number."""
    return (voice_param(params, "ALG") & 31) + 1


def carriers(params):
    return CARRIERS[algorithm(params) - 1]


def carrier_text(params):
    return " ".join("OP%d" % n for n in carriers(params))


def param_label(index):
    """Human name for a position in the 155-byte list, e.g. 'OP3 R1'."""
    if index < VOICE_BLOCK_START:
        slot, k = divmod(index, OP_BLOCK)
        return "OP%d %s" % (6 - slot, OP_PARAMS[k])
    if index < NAME_START:
        return VOICE_PARAMS[index - VOICE_BLOCK_START]
    return "NAME[%d]" % (index - NAME_START)


# ---------------------------------------------------------------------------
# Display helpers
# ---------------------------------------------------------------------------

def note_name(index):
    """Name a note where index 0 is C-1 (MIDI note 0)."""
    return "%s%d" % (NOTE_NAMES[index % 12], index // 12 - 1)


def breakpoint_name(bp):
    # Break point 0 is A-1 and 39 is C3, as on the DX7 panel.
    return note_name(bp + 9)


def transpose_name(trnsp):
    # Transpose 24 is C3, the untransposed middle setting.
    return "%s (%+d)" % (note_name(trnsp + 24), trnsp - 24)


def frequency_text(mode, coarse, fine):
    if mode == 0:
        ratio = 0.5 if coarse == 0 else coarse * (1 + fine / 100.0)
        return "ratio %.2f" % ratio
    hz = 10 ** (coarse % 4) * 10 ** (fine / 100.0)
    return "fixed %.3f Hz" % hz


def detune_text(det):
    return "%+d" % (det - 7)


def lfo_wave_name(lfw):
    return LFO_WAVES[lfw] if lfw < len(LFO_WAVES) else "?%d" % lfw


def curve_name(c):
    return CURVES[c & 3]


# ---------------------------------------------------------------------------
# SysEx parsing
# ---------------------------------------------------------------------------

class Message:
    """One SysEx message found in a file, with any problem noted."""

    def __init__(self, kind, offset):
        self.kind = kind          # "bank", "voice", "unknown", "truncated"
        self.offset = offset
        self.voices = []          # list of 155-byte parameter lists
        self.error = None         # a sentence, or None when all is well
        self.note = None          # an informational sentence


def checksum(data):
    return (-sum(data)) & 0x7F


def split_messages(blob):
    """Yield (offset, body) for each F0 ... F7 span. Truncated spans yield None."""
    pos = 0
    while True:
        start = blob.find(0xF0, pos)
        if start < 0:
            return
        end = blob.find(0xF7, start + 1)
        if end < 0:
            yield start, None
            return
        yield start, blob[start + 1:end]
        pos = end + 1


def parse_body(body, offset):
    """Classify a message body (bytes between F0 and F7) into a Message."""
    if len(body) < 5 or body[0] != 0x43 or (body[1] & 0xF0) != 0x00:
        msg = Message("unknown", offset)
        msg.note = "skipped unknown SysEx message (%d bytes)" % (len(body) + 2)
        return msg
    if body[2] == 0x09 and body[3:5] == b"\x20\x00":
        return parse_payload("bank", body[5:], BANK_DATA_SIZE, offset)
    if body[2] == 0x00 and body[3:5] == b"\x01\x1B":
        return parse_payload("voice", body[5:], UNPACKED_SIZE, offset)
    msg = Message("unknown", offset)
    msg.note = "skipped Yamaha message with format byte 0x%02X" % body[2]
    return msg


def parse_payload(kind, payload, expected, offset):
    msg = Message(kind, offset)
    if len(payload) != expected + 1:
        msg.error = "%s message has %d data bytes, expected %d" % (
            kind, max(len(payload) - 1, 0), expected)
        return msg
    data, given = payload[:expected], payload[expected]
    if checksum(data) != given:
        msg.error = "checksum mismatch (stored 0x%02X, computed 0x%02X)" % (
            given, checksum(data))
    if kind == "bank":
        msg.voices = [unpack(data[i * PACKED_SIZE:(i + 1) * PACKED_SIZE])
                      for i in range(32)]
    else:
        msg.voices = [list(data)]
    return msg


def parse_file(path):
    with open(path, "rb") as f:
        blob = f.read()
    messages = []
    for offset, body in split_messages(blob):
        if body is None:
            msg = Message("truncated", offset)
            msg.error = "truncated SysEx message at offset %d (no F7)" % offset
            messages.append(msg)
        else:
            messages.append(parse_body(body, offset))
    if not messages:
        msg = Message("truncated", 0)
        msg.error = "no SysEx messages found"
        messages.append(msg)
    return messages


def all_voices(messages):
    voices = []
    for msg in messages:
        voices.extend(msg.voices)
    return voices


def print_notes_and_errors(messages):
    """Print any notes and errors. Return True if there were errors."""
    bad = False
    for msg in messages:
        if msg.note:
            print("note: %s" % msg.note)
        if msg.error:
            print("error: %s" % msg.error)
            bad = True
    return bad


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_list(path):
    messages = parse_file(path)
    voices = all_voices(messages)
    print("%-4s %-10s  %-3s %-2s  %-22s %s" % (
        "#", "name", "alg", "fb", "carriers", "transpose"))
    for i, v in enumerate(voices, 1):
        print("%-4d %-10s  %-3d %-2d  %-22s %s" % (
            i, voice_name(v), algorithm(v), voice_param(v, "FB"),
            carrier_text(v), transpose_name(voice_param(v, "TRNSP"))))
    if not voices:
        print("(no voices)")
    return 1 if print_notes_and_errors(messages) else 0


def load_voice(path, number):
    """Return the 1-based voice from a file, or exit with a message."""
    messages = parse_file(path)
    voices = all_voices(messages)
    if number < 1 or number > len(voices):
        print("error: %s has %d voices, no voice %d" % (path, len(voices), number))
        sys.exit(1)
    for msg in messages:
        if msg.error:
            print("warning: %s" % msg.error)
    return voices[number - 1]


def cmd_voice(path, number):
    v = load_voice(path, number)
    print("Voice %d: %s" % (number, voice_name(v)))
    print()
    print_operator_table(v)
    print()
    print_voice_block(v)
    return 0


def print_operator_table(v):
    columns = ["op", "R1", "R2", "R3", "R4", "L1", "L2", "L3", "L4", "OL",
               "frequency", "det", "BP", "LD", "LC", "RD", "RC", "RS", "AMS", "KVS"]
    rows = []
    for op in range(1, 7):
        row = ["OP%d%s" % (op, "*" if op in carriers(v) else " ")]
        row += [op_param(v, op, n) for n in ("R1", "R2", "R3", "R4",
                                             "L1", "L2", "L3", "L4", "OL")]
        row.append(frequency_text(op_param(v, op, "MODE"),
                                  op_param(v, op, "FC"),
                                  op_param(v, op, "FF")))
        row.append(detune_text(op_param(v, op, "DET")))
        row.append(breakpoint_name(op_param(v, op, "BP")))
        row.append(op_param(v, op, "LD"))
        row.append(curve_name(op_param(v, op, "LC")))
        row.append(op_param(v, op, "RD"))
        row.append(curve_name(op_param(v, op, "RC")))
        row += [op_param(v, op, n) for n in ("RS", "AMS", "KVS")]
        rows.append([str(x) for x in row])
    widths = [max(len(c), *(len(r[i]) for r in rows)) for i, c in enumerate(columns)]
    print("  ".join(c.ljust(w) for c, w in zip(columns, widths)))
    for r in rows:
        print("  ".join(x.ljust(w) for x, w in zip(r, widths)))
    print("(* marks a carrier)")


def print_voice_block(v):
    g = lambda n: voice_param(v, n)
    print("Algorithm   : %d   carriers: %s" % (algorithm(v), carrier_text(v)))
    print("Feedback    : %d" % g("FB"))
    print("Osc key sync: %s" % ("on" if g("OKS") else "off"))
    print("Pitch EG    : rates %d %d %d %d   levels %d %d %d %d" % (
        g("PR1"), g("PR2"), g("PR3"), g("PR4"),
        g("PL1"), g("PL2"), g("PL3"), g("PL4")))
    print("LFO         : speed %d  delay %d  PMD %d  AMD %d  sync %s  wave %s  PMS %d" % (
        g("LFS"), g("LFD"), g("LPMD"), g("LAMD"),
        "on" if g("LKS") else "off", lfo_wave_name(g("LFW")), g("LPMS")))
    print("Transpose   : %d = %s" % (g("TRNSP"), transpose_name(g("TRNSP"))))
    print("Name        : %s" % voice_name(v))


def parse_voice_ref(ref):
    """Split 'FILE.syx:N' into (path, N)."""
    path, sep, num = ref.rpartition(":")
    if not sep or not num.isdigit():
        print("error: expected FILE.syx:N, got %r" % ref)
        sys.exit(1)
    return path, int(num)


def cmd_diff(ref_a, ref_b):
    path_a, n_a = parse_voice_ref(ref_a)
    path_b, n_b = parse_voice_ref(ref_b)
    a = load_voice(path_a, n_a)
    b = load_voice(path_b, n_b)
    print("A: %s voice %d  %s" % (path_a, n_a, voice_name(a)))
    print("B: %s voice %d  %s" % (path_b, n_b, voice_name(b)))
    differing = [i for i in range(UNPACKED_SIZE) if a[i] != b[i]]
    groups = diff_groups(differing)
    for title, indices in groups:
        print()
        print(title)
        for i in indices:
            print("  %-10s %8s   %8s" % (param_label(i).split(" ")[-1],
                                         value_text(a, i), value_text(b, i)))
    if voice_name(a) != voice_name(b):
        print()
        print("NAME         %-10r %-10r" % (voice_name(a), voice_name(b)))
    print()
    print("%d of %d bytes differ" % (len(differing), UNPACKED_SIZE))
    return 0


def diff_groups(indices):
    """Group differing byte positions by operator and voice block."""
    groups = []
    for op in range(1, 7):
        slot = 6 - op
        mine = [i for i in indices
                if slot * OP_BLOCK <= i < (slot + 1) * OP_BLOCK]
        if mine:
            groups.append(("OP%d" % op, mine))
    voice = [i for i in indices if VOICE_BLOCK_START <= i < NAME_START]
    if voice:
        groups.append(("Voice", voice))
    return groups


def value_text(v, index):
    """Show a byte in the diff with its readable meaning where that helps."""
    label = param_label(index)
    name = label.split(" ")[-1]
    raw = v[index]
    if name == "ALG":
        return str(raw + 1)
    if name == "DET":
        return detune_text(raw)
    if name == "BP":
        return "%d %s" % (raw, breakpoint_name(raw))
    if name in ("LC", "RC"):
        return curve_name(raw)
    if name == "LFW":
        return lfo_wave_name(raw)
    if name == "MODE":
        return "fixed" if raw else "ratio"
    return str(raw)


def cmd_json(path):
    messages = parse_file(path)
    out = [{"name": voice_name(v), "alg": algorithm(v), "params": list(v)}
           for v in all_voices(messages)]
    json.dump(out, sys.stdout, indent=1)
    print()
    return 1 if print_notes_and_errors(messages) else 0


# ---------------------------------------------------------------------------
# Self test
# ---------------------------------------------------------------------------

def synthetic_voice(index, rng):
    """Build one voice with every parameter inside its legal range."""
    params = []
    for _ in range(6):
        for name in OP_PARAMS:
            params.append(pick(index, OP_MAX[name], rng))
    for name in VOICE_PARAMS:
        params.append(pick(index, VOICE_MAX[name], rng))
    name = ("SYN%02d VOICE" % index)[:10].ljust(10)
    if index == 1:
        name = "~~~~~~~~~~"          # highest printable ASCII
    params.extend(ord(c) for c in name)
    return params


def pick(index, maximum, rng):
    # Voice 0 sits at every minimum, voice 1 at every maximum, the rest
    # are random so every bit of every packed field gets exercised.
    if index == 0:
        return 0
    if index == 1:
        return maximum
    return rng.randint(0, maximum)


def bank_message(packed_voices, bad_checksum=False):
    data = b"".join(packed_voices)
    cs = checksum(data)
    if bad_checksum:
        cs ^= 0x01
    return b"\xF0\x43\x00\x09\x20\x00" + data + bytes([cs]) + b"\xF7"


def voice_message(params):
    data = bytes(params)
    return b"\xF0\x43\x00\x00\x01\x1B" + data + bytes([checksum(data)]) + b"\xF7"


def run_capture(argv):
    """Run main() with argv, returning (exit_code, printed_text)."""
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        try:
            code = main(argv)
        except SystemExit as e:
            code = e.code if isinstance(e.code, int) else 1
    return code, buf.getvalue()


def cmd_selftest():
    rng = random.Random(1234)
    voices = [synthetic_voice(i, rng) for i in range(32)]
    packed = [pack(v) for v in voices]

    for v, p in zip(voices, packed):
        assert unpack(p) == v, "unpack(pack(v)) != v"
        assert pack(unpack(p)) == p, "pack(unpack(p)) != p"
    print("ok  pack/unpack round trip for 32 voices")

    with tempfile.TemporaryDirectory() as tmp:
        good = os.path.join(tmp, "good.syx")
        with open(good, "wb") as f:
            f.write(bank_message(packed) + voice_message(voices[5]))
        messages = parse_file(good)
        back = all_voices(messages)
        assert not any(m.error for m in messages), "unexpected parse error"
        assert len(back) == 33, "expected 32 bank voices plus 1 single voice"
        assert back[:32] == voices, "bank voices did not round trip"
        assert back[32] == voices[5], "single voice did not round trip"
        for v, w in zip(voices, back):
            assert voice_name(v) == voice_name(w), "name mismatch"
            assert algorithm(v) == algorithm(w), "algorithm mismatch"
        print("ok  bank + single voice file read back; names and algorithms match")

        code, text = run_capture([good])
        assert code == 0, "list of a good file should exit 0"
        assert voice_name(voices[7]) in text, "voice list is missing a name"
        print("ok  voice list exits 0 on a good file")

        code, text = run_capture([good, "--json"])
        parsed = json.loads(text)
        assert parsed[3]["params"] == voices[3], "JSON params mismatch"
        assert parsed[3]["alg"] == algorithm(voices[3]), "JSON alg mismatch"
        print("ok  JSON dump round trips")

        code, text = run_capture(["--diff", "%s:1" % good, "%s:2" % good])
        assert code == 0, "diff should exit 0"
        assert "bytes differ" in text, "diff summary missing"
        print("ok  diff runs")

        bad = os.path.join(tmp, "bad.syx")
        with open(bad, "wb") as f:
            f.write(bank_message(packed, bad_checksum=True))
        code, text = run_capture([bad])
        assert code == 1, "corrupt checksum should exit 1"
        assert "checksum mismatch" in text, "checksum error not reported"
        print("ok  corrupted checksum reported, exit code 1")

        short = os.path.join(tmp, "short.syx")
        with open(short, "wb") as f:
            f.write(bank_message(packed)[:2000])
        code, text = run_capture([short])
        assert code == 1, "truncated file should exit 1"
        assert "truncated" in text, "truncation not reported"
        print("ok  truncated file reported, exit code 1")

        odd = os.path.join(tmp, "odd.syx")
        with open(odd, "wb") as f:
            f.write(b"\xF0\x7E\x00\x06\x01\xF7" + bank_message(packed))
        code, text = run_capture([odd])
        assert code == 0 and "skipped unknown" in text, "unknown message handling"
        print("ok  unknown SysEx message skipped with a note")

    print("all selftests passed")
    return 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def build_parser():
    p = argparse.ArgumentParser(
        prog="syx_dump.py",
        description="Dump and diff DX7 voices stored in SysEx files.",
        epilog="Voice numbers are 1-based. --diff takes FILE.syx:N FILE2.syx:M.")
    p.add_argument("file", nargs="?", help="SysEx file to read")
    p.add_argument("--voice", type=int, metavar="N",
                   help="print voice N in full")
    p.add_argument("--json", action="store_true",
                   help="dump all voices as JSON")
    p.add_argument("--diff", nargs=2, metavar="FILE:N",
                   help="show only the parameters that differ between two voices")
    p.add_argument("--selftest", action="store_true",
                   help="run the built-in checks")
    return p


def main(argv=None):
    args = build_parser().parse_args(argv)
    if args.selftest:
        return cmd_selftest()
    if args.diff:
        return cmd_diff(args.diff[0], args.diff[1])
    if not args.file:
        build_parser().print_help()
        return 2
    if args.json:
        return cmd_json(args.file)
    if args.voice is not None:
        return cmd_voice(args.file, args.voice)
    return cmd_list(args.file)


if __name__ == "__main__":
    sys.exit(main())
