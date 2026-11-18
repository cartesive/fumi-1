# Upstream and reference commits

FuMi-1 is a fork of FoMni. Recorded at the start of the project (9 Oct 2026); the FoMni pin moved on 18 Nov 2026 (1.1.1).

The move to `6a01411` (1.1.1) brought FoMni's `fm1_flash_ok.h` (the UC25HQ80 flash part, fm1-base#35), the M-UPGRADE 3.11
handshake in `ota.c` / `fm1_install.py` / `fm1ota.js`, stock V16 and both flash parts in `fm1_rescue.py` / `.sh`, and
`usb.c`'s SysEx queue at 128 entries. FuMi-1 does not take FoMni's screen reader (`announce.c`), which is outside the platform.

| Repository | Role | Commit | Date |
|---|---|---|---|
| https://github.com/charlesvestal/fm1-omnichord (FoMni) | **Base.** Platform code is kept byte-for-byte (see `tools/guard_platform.sh`) | `6a0141152a4afd1eb7574c8d6be410f437b87633` (moved for 1.1.1 from `b81eb6b661c35948ec2d1059664d4e5790999305` of 2026-10-08) | 2026-11-18 |
| https://github.com/hugelton/Felucca | Read-only reference and parts bin: FM6 engine (msfa port, Apache-2.0), PHYS strings, HALL reverb, web editor protocol | `f21d6d69e88e2e70908beaa97179fa8818ccae4e` (Felucca 1.4.1) | 2026-10-09 |
| https://github.com/isod89/sloop-fm1 (SLOOP) | Read-only reference: editor protocol, KOTO preset in PHYS | `fa9ce5742f4aea9484e7c129dd81079d07e7321f` (SLOOP 2.5) | 2026-10-08 |

The reference checkouts live outside this tree, in `../fm1-refs/{Felucca,sloop-fm1,fomni-upstream}`.

The `upstream` git remote of this repository points at FoMni. `tools/guard_platform.sh` diffs the
protected paths against the commit above and fails if they differ.

## M1 record (unmodified FoMni, built here)

Built on macOS (Apple silicon) with Docker (OrbStack, linux/amd64), JieLi toolchain
`jieli-linux-toolchains-20260730.1`, SDK `AC79NN_SDK_V1.2.1_2023-12-13`:

- `tests/run_tests.sh`: 13 of 13 passed (fastmath, encoder, uac, trs, omni, storage, ota-entry,
  update-loader, installer, host-build, three scenarios).
- `web/emu/build.sh` + `tests/host/emu_test.mjs`: passed (Emscripten 6.0.12).
- `./build.sh`: `build/omni.fwsc`, 609657 B, identity `FM-1_800`, SHA-256
  `dc0759191447442db64eabbbf25f7a1e1a2a7746501f01402a7ebd1cd1446723`. App image 157760 B.

That package is the one the owner flashes by hand for the M1 round trip (custom → stock V15 →
custom) before any FuMi code is installed.
