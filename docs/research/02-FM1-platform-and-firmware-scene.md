# 02 — M-VAVE FM-1: hardware and the custom firmware scene

Researched 9 Oct 2026. The scene is moving weekly; re-check each project's page before relying on a version number here. Hardware facts marked **[code]** were read directly from the HAL source in the Felucca / FoMni repositories; **[reported]** means a community research document says so.

## The hardware you are programming

| Part | Fact | Basis |
|---|---|---|
| SoC | JieLi AC791N (family codename WL82), LQFP48 | [reported] awesome-fm-1, lunar-modulator docs/01 |
| CPU | Two pi32v2 cores (32-bit, custom ISA). Stock firmware runs at 240 MHz and renders voices on the second core | [reported] lunar-modulator docs/01 |
| FPU | Single precision only. No `double`: FoMni's build fails if a soft-double routine gets linked | [code] FoMni BUILDING.md |
| RAM | 578 KB SRAM on chip, no SDRAM | [reported] |
| Flash | 1 MiB, **single bank**, execute-in-place at `0x02000000` | [reported] + [code] |
| App slot | `0x8DFBC` bytes (about 581 KB) for code **and** any sample data | [code] FoMni `app.ld`, `fm1pkg_make.py` |
| App RAM | 96 KB general + 336 KB "pool" for big buffers (reverb lines etc.) | [code] FoMni `app.ld` |
| Display | 1.54-inch colour LCD, 240 × 240, on SPI | [code] + [reported] |
| Audio | I2S to an external codec, 44.1 kHz stereo, 24-bit samples in int32, rendered in an interrupt in 64-frame halves | [code] `fm1_audio.h`; block size [reported] |
| Note keys | **27 keys, F3 to G5: 16 white, 11 black. No velocity** (a plain switch matrix) | [code] `fm1_input.h`, `plat.h` |
| Buttons | 14, each with an LED: FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC, OCT−, OCT+ | [code] `plat.h` |
| Encoders | **7 detented rotary encoders**: SELECT, ALGORITHM, PRESETS, KNOB 1–4. One click = one step. They are not pots | [code] `fm1_input.h` |
| MASTER | The **only analogue control**: a pot on a 10-bit ADC | [code] `fm1_adc.h` |
| Key LEDs | Keys and buttons can be lit, including a dim "glow" level | [code] |
| I/O | USB-C (MIDI, and audio in the Felucca family), 3.5 mm TRS MIDI in, headphones, speaker, battery. BLE exists on the chip; FoMni never enables the radio | [code] + [reported] |
| Debug | No JTAG/SWD/UART pads, no recovery button | [reported] aroum, awesome-fm-1 |

Two consequences for the ST50 project:

1. **Knob-driven pitch bend is stepped by nature.** The encoders deliver clicks, not a continuous value. Smooth bend has to be manufactured in the audio engine. See `04-firmware-design.md`.
2. **581 KB for everything means synthesis, not sampling.** A multisampled koto and shakuhachi would not fit comfortably alongside code, and no ST-50 sample set is licensed for redistribution anyway.

## Flash layout (stock V15)

| Offset | Contents |
|---|---|
| `0x00000` | Flash header |
| `0x000A0` | SPL `uboot.boot` (second-stage bootloader) |
| `0x038D0` | `isd_config.ini` (holds the chip key) |
| **`0x04000`–`0x93000`** | **App area** (encrypted with the chip key) |
| `0x93000` | VM region (settings, BT MAC, calibration) |
| `0xE9000` | BTIF |
| `0xEA000` | USR (stock user patches) |
| `0xFC000`+ | Free; `key_mac` at `0xFF000` |

The first 16 KB (`0x0000`–`0x4000`) is "the head". If the head is damaged the unit cannot boot at all. Felucca-family update loaders are hard-coded never to write it. [reported: lunar-modulator docs/01; code: `ldr_core.c`]

## How installing works

There is no vendor bootloader button. Everything goes through the running firmware:

1. **Step 1:** the host sends an upgrade SysEx. The *running app* pulls parts of the package, checks them, stages a small update loader in flash, writes an update record and resets.
2. **Step 2:** the SPL starts that loader (it shows up on USB as `ota-…`). The loader pulls the whole package and writes the app area, sector by sector, verifying each.
3. The loader clears the record and resets into the new app.

An interrupted step 2 is resumable: the loader stays in place across power cycles and you press Install again. What is **not** recoverable by USB alone is an app that boots far enough to take over but then cannot run its update entry. That is the brick scenario, and it is why the safety design in `03-flashing-safety.md` matters.

Package contents are not authenticated (CRC-16 only), which is why custom firmware is possible at all.

## The firmwares

| Firmware | Author | Licence | Base | Notes |
|---|---|---|---|---|
| [FM-1+VA](https://baudgirl.com/work/FM-1+VA) | Baud Girl | Closed, free | Patched M-VAVE V15 | First third-party firmware. Keeps the stock FM engine, adds VA and 8-bit engines |
| [Felucca](https://github.com/hugelton/Felucca) | Leo Kuroshita, Hügelton | GPL-3.0 | Clean-room | **The platform everyone forks.** 13 engines, 4 tracks, web installer, web editor, browser emulator (WebAssembly), host test suite |
| [SLOOP](https://github.com/isod89/sloop-fm1) | 3dSam (isod89) | GPL-3.0 | Felucca fork | 4-track live groovebox. Web installer and editor. Adds a boot guard and "USB rescue" (hold OCT− at power-on) |
| [X0X](https://github.com/charlesvestal/fm1-x0x) | Charles Vestal | GPL-3.0 | Felucca platform | 909/808 + two 303s. Introduced a `plat.h` layer and host simulator |
| [OMNI / FoMni](https://github.com/charlesvestal/fm1-omnichord) | Charles Vestal | GPL-3.0 | Felucca platform via X0X | **The Omnichord clone.** A single instrument, about 6,400 lines including platform |
| [Jangada](https://github.com/zednaked/jangada), [Melodee](https://github.com/keremimo/melodee), SLOOP ALG, sloopDX, zp12, ChoralRoot, Hortator, FiMba-1 (a kalimba), GHOULBOX | various | GPL-3.0 | Felucca / SLOOP forks | See the hub below |
| [Groove OS](https://www.groove-os.com/) | Peter Gombos | Commercial ($29) | Patched V15 | 8-track groovebox |

Useful indexes: [awesome-fm-1](https://github.com/cicloid/awesome-fm-1) (the best link list), the [FM-1 Firmware Hub](https://fm1.designburgapps.com/) (one-page web installer for the GPL firmwares, and for stock V15), and [Drey Andersson's ranking](https://dreyandersson.com/blog/m-vave-fm-1-custom-firmware/).

## What SLOOP and Felucca give you

- **Web installer** (Chrome or Edge, Web MIDI): `web/fm1ota.js` + `web/fm1pkg.js` implement the update protocol in the browser. It verifies the package by SHA-256 before sending, can finish an interrupted install, and has a "Return to official V15" path that accepts only M-VAVE's exact file.
- **Web editor**: a SysEx protocol documented in `web/EDITOR_PROTOCOL.md` (manufacturer ID `7D`, header `F0 7D 46 4C <cmd> …`). Parameter get/set/describe, dumps, user presets, sample upload, live "watch" pushes. One request at a time, 10–50 ms replies, frames under 640 bytes.
- **Command-line installer**: `tools/fm1_install.py PACKAGE.fwsc`, and `--info` to print the connected unit's identity.
- **Host tests**: storage, update entry and loader, MIDI parser, DSP renders hashed against golden files, CPU-cost budgets.
- **Build**: `./build.sh` → `build/<name>.fwsc`. Needs the JieLi toolchain (clang 4.0.1 for pi32v2, **Linux x86-64 only**; Docker on macOS, WSL on Windows) and three files from the JieLi AC79 SDK (`uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin`, tag `AC79NN_SDK_V1.2.1_2023-12-13` from Gitee).

## Why FoMni is the better starting point than SLOOP

SLOOP is a groovebox with nine engines, a sequencer, a song mode and an editor. Almost none of that is wanted in an ST50, and all of it would have to be understood before it could be removed safely.

FoMni is the same shape as the ST50: one instrument, keys that are not a piano, a handful of pages.

- `firmware/src/dsp/omni.c` (782 lines) is the whole instrument. Replace it.
- `firmware/src/app/ui.c` (786 lines) is the whole UI. Replace it.
- `firmware/src/app/plat.h` (49 lines) is everything the app needs from the hardware: `plat_keys()`, `plat_buttons()`, `plat_enc()`, `plat_master()`, `plat_leds()`, MIDI in/out, storage, CPU load.
- `host/omni_host.c` runs the whole app on a computer from a script, writing audio and screenshots. `web/emu/` builds the same code for the browser.
- It already has the safety net: watchdog armed first, boot-loop guard, **safe mode** after two failed boots, the chip's own update mode after four, OCT− + OCT+ held 5 s for update mode, `tools/fm1_rescue.py`.
- It already has a web installer page and a publishing script for GitHub Pages.

What FoMni lacks is a web editor. Borrow SLOOP's/Felucca's protocol and page for that later (milestone M7).

Things worth lifting from Felucca (all GPL-3.0-compatible):

- `eng_phys.c` / `phys_dsp.c`: plucked-string and modal models ported from DaisySP and Mutable Instruments Rings (MIT). Candidate koto voice.
- `eng_formant.c`: formant voice after klattsch (MIT). Candidate "chorus" voice.
- `fx.c`: HALL reverb, chorus.
- `tests/pitch.py`: a pitch checker, useful for tuning tests.
- The global TUNE parameter (±50 cents) and MIDI pitch-bend handling.

## Licence consequences

Felucca, SLOOP, X0X and FoMni are all **GPL-3.0-only**. A fork is GPL-3.0 too: if you distribute the firmware you must publish complete source. Keep the upstream copyright headers. The three JieLi SDK files are Apache-2.0 and are read from your SDK checkout at build time, not committed. Do not commit M-VAVE's `.fwsc`. "Suiko" and "ST-50" are someone else's names: say "inspired by", as FoMni does for Omnichord.

## Sources

- https://github.com/hugelton/Felucca (README, BUILDING.md, `firmware/hal/`, `firmware/loader/`, `tools/`, `web/`)
- https://github.com/isod89/sloop-fm1 (README, BUILDING.md, `web/EDITOR_PROTOCOL.md`)
- https://github.com/charlesvestal/fm1-omnichord (README, BUILDING.md, LICENSING.md, `firmware/`)
- https://github.com/ip2k/lunar-modulator (docs/01 hardware, docs/03 update protocol, docs/07 recovery and risk)
- https://github.com/cicloid/awesome-fm-1
- https://github.com/aroum/fm1-custom-fw
- https://github.com/AL-255/FM-1-RE (docs/io/11-ota-protocol.md)
- https://fm1.designburgapps.com/
- https://dreyandersson.com/blog/m-vave-fm-1-custom-firmware/
