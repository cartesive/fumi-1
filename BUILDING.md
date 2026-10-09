# Building FuMi-1

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `fumi.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `fumi.fwsc` | the installable package (app + loader) |

## Prerequisites (macOS)

- Python 3.10+ with Pillow (with raqm), numpy and scipy. On macOS the system Python's Pillow lacks raqm;
  use a venv from Homebrew's Python (`docs/dev-environment.md`) and put its `bin` first on `PATH`
- Docker Desktop. The JieLi toolchain is Linux x86-64 only; the build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder Docker can share, e.g. under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).
- Emscripten (optional: the browser emulator and the audition bench). The official emsdk works; it needs
  Python 3.10+ on PATH.

On Linux x86-64 the toolchain runs natively and Docker is not needed.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 0.1` makes a release build; the package is `build/fumi-0.1.fwsc` (identity
`FM-1_7000100`; dev builds are `FM-1_700`).

On macOS with podman instead of Docker, put a `docker` script that runs `exec podman "$@"` first
on your PATH. `OM_JOBS` (default 4) limits parallel compiles: a podman machine drops
connections when many containers start at once.

Build option: `OM_CDC=1` adds Felucca's USB serial function (off by default: one plain
MIDI interface).

The build also generates `build/gen/` (the font). It fails if a soft-double routine is linked
(a `double` crept in) and checks the image, RAM and pool sizes.

## Tests

```
tests/run_tests.sh
```

Runs, on the build machine: the platform guard (`tools/guard_platform.sh`); the maths library against
libm; the pitch core (`tuning`, `smooth`, `bend`); the FM6 core; the instrument; the click regression test
and the proof that it fails without smoothing; the no-double check; flash storage; Felucca's update-path
tests against `build/fumi.fwsc`; the whole app in the simulator (`tests/scenarios/*.fumi`, with screenshots
and audio in `build/scenarios/`); the browser build and the bench exports in Node; and the Python tools'
self-tests.

`host/build_host.sh` builds the simulator alone; `build/host/fumi_host SCRIPT OUTDIR` runs one
script (the command list is at the top of `host/fumi_host.c`).

## Install

**Before the first flash of your own build**, go through `docs/research/03-flashing-safety.md`. The short
form:

- The unit reports a known-good identity: `python3 tools/fm1_install.py --info` (stock V15 is `FM-1_015`;
  FuMi-1 1.0.3 is `FM-1_7010003`). Custom firmware expects the unit to have been on stock V15.
- The official `FM-1.fwsc` (V15) is on this computer and its SHA-256 is
  `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`. It is the way home; the installer
  page's "Back to the stock firmware" takes exactly that file.
- You have installed your own build of unmodified FoMni and done one round trip, custom → V15 → custom
  (`docs/upstream.md`, the M1 record). That proves the toolchain and the packaging before any FuMi code
  is involved.
- Battery charged; a direct USB data cable, no hub; Chrome or Edge; no DAW or other MIDI app open.
- The package comes from a clean tree with `tests/run_tests.sh` green, including the update-path tests
  (`ota-entry`, `update-loader`, `installer`, which need `AC79_SDK`).

The web installer is the `install/` page of the site (`tools/make_pages.py`); from the command line
(needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/fumi.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Do not unplug until the installer says Done and the unit has restarted. Then: check the identity, hold
OCT− and OCT+ for 3 s to see the update-mode countdown appear (release before 5 s), and play.

Installing firmware is at your own risk. Hold OCT− and OCT+ for 5 seconds for Felucca's update
mode. Two failed boots in a row put FuMi-1 in safe mode (no audio, USB on, installable). If the FM-1 no
longer starts but reaches the chip's update mode (4C4A:8057 on USB), `tools/fm1_rescue.sh` puts stock
firmware back from a Mac; otherwise recovery needs
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Publishing a release

1. `./build.sh --release X.Y` (the identity, FM-1_7XXYYZZ, is what the installer checks; 7xx was free on
   awesome-fm-1 at 0.1).
2. `AC79_SDK=~/fw-AC79_AIoT_SDK tests/run_tests.sh`, all green, and the three update-path tests run by hand
   against the release package too (`tests/ota_test.c`, `tests/ldr_test.c`, `tests/install_test.py`).
3. `tools/publish_pages.sh X.Y`: builds the site (landing page, browser emulator, audition bench, web
   installer, firmware download) with `tools/make_pages.py` and pushes it to the `gh-pages` branch.
4. `git tag vX.Y` and `gh release create vX.Y build/fumi-X.Y.fwsc` for the command-line download, with the
   package's SHA-256 in the notes.
