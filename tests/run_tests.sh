#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# FuMi-1 host tests: the platform guard, the maths, the platform pieces kept from Felucca / X0X / FoMni,
# the pitch core, the FM6 core, the instrument, the click regression test (and its proof), the no-double
# check, flash storage, the update path against the package, the simulator scenarios (tests/scenarios/*.fumi:
# screenshots and audio in build/scenarios/), the browser build, and the Python tools' self-tests.
#   tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
PY="${PYTHON:-python3}"
FAIL=0
OUT=build/host
mkdir -p "$OUT" build/scenarios
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -15 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
DSP="-O2 -ffp-contract=off -std=c99 -Wall -Wextra -Werror -Wno-unused-function -DFM_HOST -Ifirmware/src/dsp"
run guard sh tools/guard_platform.sh
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
for t in tuning smooth bend fm6 fumi click; do
    run "$t" sh -c "$CC $DSP -o $OUT/${t}_test tests/host/${t}_test.c -lm && $OUT/${t}_test"
done
# the click test must fail with the smoothing bypassed, or it tests nothing
run click-proof sh -c "$CC $DSP -DFM_NO_SMOOTH -o $OUT/click_nosmooth tests/host/click_test.c -lm && ! $OUT/click_nosmooth"
# no double anywhere in the firmware's own code (the FPU is single precision)
run no-double sh -c "$CC -fsyntax-only -std=c99 -Wall -Wextra -Wdouble-promotion -Werror -Wno-unused-function -DFM_HOST -Ifirmware/src/dsp firmware/src/dsp/fumi.c && $PY tools/check_no_double.py firmware/src/dsp/*.c firmware/src/dsp/*.h firmware/src/app/*.c firmware/src/app/*.h"
run tables sh -c "$PY tools/gen_fm6_tables.py $OUT/fm6_tables.h >/dev/null && cmp $OUT/fm6_tables.h firmware/src/dsp/fm6_tables.h && $PY tools/fumi_patches.py $OUT/fumi_patches.h >/dev/null && cmp $OUT/fumi_patches.h firmware/src/dsp/fumi_patches.h"
run storage sh -c "$CC -O2 -o $OUT/storage_test tests/storage_test.c && $OUT/storage_test"
# the update path, against the firmware package (Felucca's tests; needs ./build.sh)
if [ -f build/fumi.fwsc ]; then
    run ota-entry sh -c "$CC -o $OUT/ota_test tests/ota_test.c && $OUT/ota_test build/fumi.fwsc"
    if [ -z "${AC79_SDK:-}" ]; then
        echo "  skip update-loader (needs AC79_SDK, as the build)"
    else
    run update-loader sh -c "head -c 100000 build/fumi.bin > $OUT/old_app.bin && \
        $PY tools/fm1pkg_make.py $OUT/old_app.bin build/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -o $OUT/ldr_test tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc build/fumi.fwsc"
    fi
    run installer $PY tests/install_test.py
else
    echo "  skip update-path tests (no build/fumi.fwsc: run ./build.sh)"
fi
run host-build sh host/build_host.sh
if command -v emcc >/dev/null 2>&1; then
    run emu sh -c "sh web/emu/build.sh >/dev/null && node tests/host/emu_test.mjs build/emu/fumi.wasm"
    run bench node tests/host/bench_test.mjs build/emu/fumi.wasm
    run syx-js node tests/host/syx_test.mjs
    run session-js node tests/host/session_test.mjs
fi
for s in tests/scenarios/*.fumi; do
    n=$(basename "$s" .fumi)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/fumi_host "$s" "build/scenarios/$n"
done
run syx-dump $PY tools/syx_dump.py --selftest
run analyze-ref $PY tools/analyze_ref.py --selftest
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
