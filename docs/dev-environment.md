# Development environment (macOS, Apple silicon)

Everything below was set up on 9 Oct 2026 and is what the build scripts expect.

| Need | Where | Why |
|---|---|---|
| Python 3.13 venv with Pillow, numpy, scipy, mido, python-rtmidi | `~/.venvs/fumi` | The system Python 3.9's Pillow lacks raqm, which `tools/gen_font.py` needs. Put `~/.venvs/fumi/bin` first on `PATH` (or `export PYTHON=~/.venvs/fumi/bin/python3`) before building |
| Emscripten | `~/emsdk` (`source ~/emsdk/emsdk_env.sh`) | The browser emulator and the audition bench. Homebrew's formula was blocked by a lock; the official emsdk works. emsdk itself needs Python ≥ 3.10 on PATH (the venv) |
| JieLi toolchain | `~/.jieli/toolchain` (from `tools/get_toolchain.sh`) | Firmware build, inside Docker (linux/amd64) |
| JieLi AC79 SDK | `~/fw-AC79_AIoT_SDK` (tag `AC79NN_SDK_V1.2.1_2023-12-13`) | Three files go into every package. Set `AC79_SDK=~/fw-AC79_AIoT_SDK` for the update-loader test |
| Docker | OrbStack | `./build.sh` runs the toolchain in a `debian:bookworm-slim` amd64 container |

A shell for building:

```
export PATH=$HOME/.venvs/fumi/bin:$PATH
export AC79_SDK=$HOME/fw-AC79_AIoT_SDK
source ~/emsdk/emsdk_env.sh
tests/run_tests.sh      # host tests, simulator scenarios, emulator
./build.sh              # build/fumi.fwsc (Docker)
```

Archive the toolchain tarball and the SDK checkout privately: if either download disappears the
firmware cannot be built.
