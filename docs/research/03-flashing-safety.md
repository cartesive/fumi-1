# 03 — Not bricking the FM-1

Researched 9 Oct 2026. Read this before the first flash of your own build, and again before each release.

## The V14 / V15 question

**Short answer: treat stock V15 as mandatory before installing any custom firmware. Nobody has documented a brick caused specifically by starting from V14, but every project assumes V15 and nobody has tested the V14 path.**

What the evidence says:

- Every guide says V15 first. awesome-fm-1: custom firmwares "expect you to be on V15 first". Drey Andersson's roundup: "all seven expect stock V15 on the unit first". The fuleo guide says the same of FM-1+VA and Felucca.
- **V15 changed the flash layout.** In V14 the app area ends at `0x94000`; in V15 it ends at `0x93000` and the settings region starts there instead. The bootloader, `ota.bin`, `cfg` and `isd_config.ini` are byte-identical between V14 and V15. [lunar-modulator docs/01 and docs/03, marked verified by that project]
- The community tooling is written around V15's layout: Felucca-family loaders write exactly `0x4000`–`0x93000`; the rescue tools compare the protected head against V15; "return to stock" accepts only the exact V15 file by SHA-256; Baud Girl's installer refuses a package whose head differs from V15's.
- So from V14 you would be on a path whose layout assumptions are off by one sector and that no developer has exercised. It may well work. There is no reason to find out on your only unit.
- I found no first-hand report of a V14-origin brick. The brick reports that exist are about other things: an update between two Felucca versions that left a unit in the chip's boot mode (Felucca issue #61, open), and an X0X brightness setting that froze units until 0.10.

One trap: **do not trust the updater's file name or the app's version label.** As of 6 Sep 2026 the macOS M-UPGRADE disk image still had V14 embedded; V15 had to be fetched separately. [lunar-modulator docs/09] Read the identity from the device instead.

### How to check what you are on

```
pip3 install mido python-rtmidi
python3 tools/fm1_install.py --info
```

(`tools/fm1_install.py` is in the Felucca, SLOOP and FoMni repositories.) Stock V15 reports **`FM-1_015`**. V14 reports `FM-1_014`. Custom firmwares report their own numbers: Baud Girl `FM-1_020` upward, Felucca family `FM-1_9xx`.

Since you have already been running FM-1+VA, SLOOP and the Omnichord firmware, your unit has already been through this path. The check still matters whenever you return to stock: confirm it reads `FM-1_015`.

### Getting to V15

- From M-VAVE: m-vave.com/download → PC Firmware → FM-1 V15 (`FM-1.fwsc`), installed with M-UPGRADE, or with any of the web installers' "Return to official V15".
- From Linux: [fm1-linux-update](https://github.com/fuleo/fm1-linux-update).
- **Keep that `FM-1.fwsc` file permanently.** It is your way home. Felucca's installer expects SHA-256 `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`.

## Why this device is easy to brick

- **One flash bank.** There is no second copy of the firmware to fall back to.
- **No recovery button, no debug header.**
- **The update entry lives inside the app.** Step 1 of an install is performed by whatever firmware is currently running. If your firmware boots but cannot run its update entry, USB installers cannot reach it.
- The only route below that is the chip's mask-ROM USB mode ("UBOOT"), which needs either a firmware that deliberately jumps into it, or a hardware dongle at power-on.

## Design rules for the ST50 firmware

These are the rules the working firmwares follow. They are ordered by how much they matter.

1. **Do not touch the platform's update and boot code.** Keep `firmware/loader/`, `firmware/src/ota.c`, `firmware/hal/`, `crt0.S`, `tools/fm1pkg_make.py` and `web/fm1ota.js` / `fm1pkg.js` byte-for-byte as upstream. Put all ST50 work in `firmware/src/app/` and `firmware/src/dsp/`. A CI check that diffs those paths against the upstream commit is cheap and worth having.
2. **Never write the flash head (`0x0000`–`0x4000`).** The upstream loader already refuses to (`FL_RANGE_OK`). Do not add any flash write of your own outside the storage API (`plat_store_save`).
3. **Keep every escape hatch, and keep them early.**
   - Watchdog armed as the first thing at boot.
   - Boot-loop guard: a crash or hang in the first 30 s counts as a failed boot. Two failures → safe mode (no audio, USB on, installable). Four → the chip's own update mode.
   - OCT− + OCT+ held 5 s → update mode. Do not reassign that combination to anything.
   - The USB upgrade SysEx handler (`ota_service()`) must run in the main loop on every page and in every mode.
4. **The audio interrupt must never be able to hang or overrun.** A stuck ISR starves the main loop that services USB. Bound every loop, no allocation, no `double`, and watch `plat_cpu_pct()` / `plat_xruns()`.
5. **Storage must tolerate garbage.** Anything read from flash (saved setups, recordings) is validated by length and CRC, and a bad object falls back to defaults. A firmware that crashes on its own saved data at start-up is the classic self-inflicted brick. The upstream storage is A/B-sectored and torn-write safe; use it rather than writing your own.
6. **A new setting must never be able to make the unit unusable.** X0X's brightness freeze is the cautionary tale. No setting may turn off the screen, LEDs and input together, and safe mode must ignore saved settings.
7. **Own identity number.** Pick a product identity nobody else uses and never reuse a number for a different build. In use today: M-VAVE up to `FM-1_019`, Baud Girl `020`+, Lunar Modulator plans `5xx`, FoMni `8xx…`, Felucca and SLOOP `9xx`. Something in `6xx` or `7xx` appears free; confirm before release.
8. **Run the upstream update tests against your package on every build**: `ldr_test`, `ota_test`, `install_test.py`, `storage_test`. They simulate the whole install on the build machine.
9. **Emulator before hardware.** Nothing gets flashed that has not booted, played and survived a scripted scenario in the host simulator.

## Flashing procedure

Before the first flash of a self-built package:

- [ ] Unit currently reports a known-good identity (`--info`).
- [ ] Official `FM-1.fwsc` (V15) is on this computer and its SHA-256 matches.
- [ ] Anything you care about on the current firmware is backed up (stock user patches do not survive a move to the Felucca family; each firmware's projects are unreadable by the others).
- [ ] You have **built upstream FoMni unmodified and installed that build** successfully. This proves your toolchain, your SDK files and your packaging before any of your own code is involved.
- [ ] You have done one full round trip on this unit: custom → official V15 → custom.
- [ ] Recovery hardware is on the desk (see below), or you accept the risk on a $70 device.

Every flash:

- [ ] Battery charged. Power loss mid-write is the worst case.
- [ ] Direct USB data cable, no hub.
- [ ] Chrome or Edge. Close DAWs, other MIDI apps and other browser tabs that hold the MIDI port.
- [ ] Package built from a clean tree; host tests green; update-path tests green.
- [ ] Do not unplug until the installer says Done and the unit has restarted.
- [ ] After restart: check identity, hold OCT− + OCT+ for 3 s to see the update-mode countdown appear (then release), confirm audio.

Switching between different custom firmwares: go through stock V15 in between rather than directly. Firmwares may not read each other's saved data, and stale data is the suspected cause of at least one black-screen report.

## Recovery ladder

Try in order.

1. **Install was interrupted, screen shows update mode or is blank but USB shows an `ota-…` MIDI device:** open the installer and press Install again. It resumes.
2. **Firmware starts but misbehaves:** hold OCT− + OCT+ for 5 s (Felucca family) for update mode, then install. SLOOP and its forks: hold OCT− alone while switching on ("USB rescue").
3. **Firmware crashes at start:** power-cycle twice. FoMni, X0X and Jangada drop into safe mode after two failed boots; install from there.
4. **Black screen, computer sees `WL80UBOOT` / `UBOOT1.00` (USB ID 4C4A:8057):** the chip is in its own download mode. That is good news. Try another cable and close every MIDI app first. Then `tools/fm1_rescue.py FM-1.fwsc` (in FoMni and X0X; macOS, no extra hardware): it backs up the whole flash, refuses to proceed if the head differs from V15, and rewrites only the app sectors that differ.
5. **Black screen, nothing on USB:** [FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter). A Seeed XIAO RP2040 with three wires to the FM-1's USB lines (D+, D−, GND; **do not connect VBUS**) forces the chip into download mode at power-on, dumps the 1 MiB flash in about 3 s and writes back only changed sectors in `0x4000`–`0x93000`. Mac host. Dump first, always.

The XIAO RP2040 costs a few dollars. Having one built and tested before you flash your own code turns "bricked" into "ten minutes". The alternative insurance is a second FM-1.

Do not type raw SysEx at the unit from guides or memory. The upgrade command and the "jump to mask ROM" soft key differ by one byte.

## Sources

- https://github.com/cicloid/awesome-fm-1 ("expect you to be on V15 first"; single bank, no debug header)
- https://dreyandersson.com/blog/m-vave-fm-1-custom-firmware/ (V15 requirement, identity strings, recovery steps, brick reports)
- https://fuleo.github.io/fm1-guide/ and https://github.com/fuleo/fm1-linux-update
- https://github.com/ip2k/lunar-modulator — docs/01-hardware.md (layout, V15 boundary move), docs/03-update-protocol.md (head, identity ranges, resumable step 2), docs/07-recovery-and-risk.md (risk register, rules), docs/09 (M-UPGRADE DMG embedding V14)
- https://github.com/hugelton/Felucca — README "If the FM-1 does not start", `tools/fm1_install.py`, `firmware/loader/ldr_core.c`, issue #61
- https://github.com/charlesvestal/fm1-omnichord — `firmware/src/app/main_fm1.c` (boot guard, safe mode, update-mode combo), `tools/fm1_rescue.py`
- https://github.com/isod89/sloop-fm1 — README "Rescue"
- https://github.com/kurogedelic/FM-1-transporter
- https://github.com/aroum/fm1-custom-fw (bootloader re-entry warning)
- https://github.com/AL-255/FM-1-RE/blob/main/docs/io/11-ota-protocol.md
- https://github.com/czietz/fm1-firmware-patcher (the one workflow that deliberately uses V14, as a downgrade step for M-UPGRADE)
