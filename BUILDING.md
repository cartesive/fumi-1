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

From the command line (needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/fumi.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Installing firmware is at your own risk. Hold OCT− and OCT+ for 5 seconds for Felucca's update
mode. If the FM-1 no longer starts but reaches the chip's update mode (4C4A:8057 on USB),
`tools/fm1_rescue.sh` puts stock firmware back from a Mac; otherwise recovery needs
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Publishing a release

1. `./build.sh --release X.Y` (the identity, FM-1_7XXYYZZ, is what the installer checks; confirm 7xx is
   still free against awesome-fm-1 and the FM-1 Firmware Hub first).
2. `tools/publish_pages.sh X.Y`: builds the site (landing page, browser emulator, web installer,
   firmware) with `tools/make_pages.py` and pushes it to the `gh-pages` branch (M7: the pages still carry
   FoMni's text and need rebranding before a first publish).
3. `gh release create vX.Y build/fumi-X.Y.fwsc` for the command-line download.
