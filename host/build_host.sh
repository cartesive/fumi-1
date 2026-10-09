#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the FuMi-1 host simulator (the whole app on this machine) into build/host/fumi_host.
# Same sources as the firmware, -ffp-contract=off like the device (no fused multiply-add).
set -e
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
# -D_POSIX_C_SOURCE: clock_gettime under -std=c99 on glibc (the build failed on Linux, 1.0.3)
$CC -O2 -ffp-contract=off -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DOM_HOST -DFM_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o build/host/fumi_host host/fumi_host.c \
    firmware/src/dsp/fumi.c -lm
echo "host: build/host/fumi_host"
