#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# FuMi-1 in the browser: the host simulator compiled to WebAssembly (Emscripten), plus its page, and the
# same wasm for the audition bench.
#   [FM_VERSION=0.1] web/emu/build.sh  ->  build/emu/{index.html, worklet.js, fumi.wasm}, build/bench/
# Same sources and flags as host/build_host.sh (-ffp-contract=off, like the device).
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/emu build/bench
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
emcc -O2 -ffp-contract=off -std=gnu99 -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -DOM_HOST -DFM_HOST -DOM_WEB "-DFM_VERSION=\"$(printf %s "${FM_VERSION:-DEV}" | tr a-z A-Z)\"" -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/emu/fumi.wasm web/emu/fumi_web.c firmware/src/dsp/fumi.c
cp web/emu/index.html web/emu/worklet.js build/emu/
if [ -d web/bench ]; then
    cp web/bench/* build/bench/
    cp build/emu/fumi.wasm build/bench/
fi
echo "emu: build/emu ($(wc -c < build/emu/fumi.wasm) B wasm)"
