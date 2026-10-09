#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No `double` in FuMi's own firmware code (the AC79's FPU is single precision; a double links soft-float
routines and the build fails late). Comments and strings are stripped first, then the word is searched.

  tools/check_no_double.py FILE...     exit 1 and a list if any file uses it
"""
import re
import sys

bad = 0
for path in sys.argv[1:]:
    src = open(path, encoding="utf-8").read()
    src = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    src = re.sub(r'"(\\.|[^"\\])*"', '""', src)
    for no, line in enumerate(src.splitlines(), 1):
        if re.search(r"\bdouble\b", line):
            print(f"{path}:{no}: double")
            bad += 1
sys.exit(1 if bad else 0)
