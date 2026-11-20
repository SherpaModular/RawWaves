# Raw Waves v2 — Hardware Platform Guide

Context for working on this repo. Everything below was derived from
`Hardware/KiCad/RawWavesV2.kicad_sch` / `.kicad_pcb` (KiCad 7 format, single
sheet) and cross-checked against the firmware in `Software/RawWavesV2/`.
If you change the schematic, re-verify this file.

## What it is

A 4HP Eurorack sample player (Radio Music / Raw Waves descendant) by Sherpa
Modular. A **Teensy 4.0** streams RAW/WAV files from a microSD card to a
**PT8211** 16-bit I²S DAC, followed by a TL072 filter/gain stage. Two pots and
two CV inputs select Station (bank/file) and Start position; a Reset button and
Reset trigger input restart playback.

## Repo layout

- `Hardware/KiCad/` — schematic, PCB, project (single sheet, 2-layer board)
- `Hardware/BOM.md` — bill of materials (see discrepancy note below)
- `Software/RawWavesV2/` — Arduino/Teensyduino firmware; `.ino` must match its folder name
- `.github/workflows/compile.yml` — CI compile for `teensy:avr:teensy40`

Handy commands (kicad-cli is available locally):

```sh
kicad-cli sch export netlist --format kicadsexpr -o /tmp/x.net Hardware/KiCad/RawWavesV2.kicad_sch
kicad-cli sch export pdf  -o /tmp/sch.pdf Hardware/KiCad/RawWavesV2.kicad_sch
```

## Teensy 4.0 pin map (authoritative)

| Teensy pin | Net | Connects to | Firmware symbol |
|---|---|---|---|
| 14 / A0 | (wiper) | RV1 Station pot wiper | `CHAN_POT_PIN` (Interface.h) |
| 15 / A1 | STATION_CV_PROTECTED | U5B buffer out ← J1 Station CV | `CHAN_CV_PIN` |
| 19 / A5 | (wiper) | RV2 Start pot wiper | `TIME_POT_PIN` |
| 22 / A8 | START_CV_PROTECTED | U5A buffer out ← J6 Start CV | `TIME_CV_PIN` |
| 8 | RESET_BUTTON | SW1 to GND (no ext. pull-up) | `RESET_BUTTON`, `INPUT_PULLUP` |
| 9 | RESET_CV_PROTECTED | Q1 collector, R15 10k pull-up to 3.3V | `RESET_CV`, interrupt |
| 5 | LED1 | LED1 (leftmost bank LED) | `LED1` (LedControl.h) |
| 4 | LED2 | LED2 | `LED2` |
| 3 | LED3 | LED3 | `LED3` |
| 2 | LED4 | LED4 (rightmost bank LED) | `LED4` |
| 6 | LED_RESET | RESET_LED1 | `RESET_LED` |
| 10 | SD DAT3/CD | microSD CS | `SS` in `SD.begin(SS)` |
| 11 | SD CMD | microSD MOSI | `SPI.setMOSI(11)` |
| 12 | SD DAT0 | microSD MISO | — |
| 13 | SD CLK | microSD SCK (also Teensy onboard LED) | `SPI.setSCK(13)` |
| 7 | DAC_DATA_IN | PT8211 DIN (I²S OUT1A) | `AudioOutputPT8211` |
| 20 | DAC_WORD_SELECT | PT8211 WS (LRCLK1) | `AudioOutputPT8211` |
| 21 | DAC_BIT_CLOCK | PT8211 BCK (BCLK1) | `AudioOutputPT8211` |
| VIN | 5.0V | LM1117-5.0 output | — |
| 3V3 | 3.3V | Teensy's own regulator feeds the 3.3V rail | — |

**Unused / free pins:** 0, 1, 16 (A2), 17 (A3), 18 (A4), 23 (A9, MCLK1), 24–39,
plus PROGRAM, ON/OFF, VBAT, USB D+/D−. Pins 23 and 33 (MCLK) are unconnected —
the PT8211 needs no MCLK. The SD card uses the **SPI pins (10–13)**, not the
Teensy 4.0's bottom-pad SDIO pins, so `BUILTIN_SDCARD` does not apply.

**LED naming:** firmware `LED1`..`LED4` match the PCB designators, left→right
on the panel. In `LedControl.cpp`, bit 0 / index 0 drives `LED1` (leftmost).

## Circuit blocks

### Power (J4, 10-pin Eurorack header)
- +12V: J4 → F1 polyfuse → D7 1N5819 in series (reverse-polarity block) → +12V rail.
- −12V: J4 → F2 polyfuse → D8 1N5819 in series → −12V rail.
- Bulk/decoupling: C9/C4 10µF + C10/C11 100nF on ±12V.
- +12V → U4 LM1117MP-5.0 → **5.0V rail** (C3 10µF, C1 100nF) → Teensy VIN and PT8211 VDD.
- **3.3V rail** comes from the Teensy's onboard regulator (3V3 pins) and feeds
  the pots, MCP6002, microSD, and the reset pull-up (C2 100nF).
- ±12V is used only by the TL072 output stage. Spec: ~100 mA on +12V, ~10 mA on −12V.
- `VUSB` is not connected in the schematic, but on a stock Teensy 4.0 VIN and
  VUSB are joined by a cuttable trace. Whether builders cut it is not captured
  here — be careful about USB + rack power at the same time.
- Back silkscreen "RED" labels mark the −12V stripe side near J4/fuses.

### Station / Start CV inputs (J1, J6 → U5 MCP6002 on 3.3V)
Identical for both channels:
`jack tip → 100k (R1/R12) → op-amp +in, 200k (R17/R18) to GND` — a 2/3 divider
mapping 0–5V CV to 0–3.33V. Each MCP6002 half is a unity-gain follower (100k
R19/R20 from output to −in). Because the MCP6002 runs on 0–3.3V and is
rail-to-rail, its output physically can't drive the ADC pin outside its rails —
this is the over/undervoltage protection. Input impedance ≈ 300k.
Jack switch pins (pin 2) are tied to GND, so unplugged inputs read 0V.
- Station CV: J1 → R1/R17 → U5B (pins 5/6/7) → Teensy 15 / A1
- Start CV:   J6 → R12/R18 → U5A (pins 3/2/1) → Teensy 22 / A8

Pots RV1 (Station) and RV2 (Start) are B50k between GND and 3.3V, wipers direct
to A0 / A5. Firmware uses a 10-bit ADC (`ADC_BITS` in AnalogInput.h), so full
scale is 0–1023; `AudioEngine::skipTo` depends on that value.

### Reset CV input (J2 → Q1)
`jack tip → R16 100k → Q1 MMBT3904 base; emitter GND; collector → pin 9 with
R15 10k pull-up to 3.3V`. This is an **inverting** open-collector stage: a high
trigger pulls pin 9 LOW. Firmware attaches the interrupt on `RISING`
(Interface.cpp), so reset actually fires on the **trailing (falling) edge** of
the incoming trigger. There is no base pull-down or base clamp diode;
the 100k limits current for negative inputs.

### Reset button and LEDs
- SW1 (E-Switch KS01Q01) shorts pin 8 to GND; firmware enables the internal pull-up (Bounce2).
- All LEDs: Teensy pin → LED anode, cathode → 10k (R2–R5, R14) → GND. At 3.3V
  this is ~0.1–0.15 mA, which is why the BOM calls for high-lumen 3mm LEDs.

### microSD (J3, Yamaichi vertical socket)
SPI mode on 3.3V: CS=10 (DAT3/CD), MOSI=11 (CMD), MISO=12 (DAT0), SCK=13 (CLK).
DAT1/DAT2 are unconnected; no external pull-ups; no card-detect switch is used.

### Audio output (U2 PT8211 → U6 TL072 → J5)
1. **PT8211** (5V supply) receives I²S on pins 7/20/21. Only the **right channel
   (RCH)** is used; LCH is unconnected. Firmware outputs mono, so this is fine,
   but don't route stereo to the left channel and expect sound.
2. **C7 100nF** AC-couples RCH.
3. **U6A: unity-gain Sallen-Key 2-pole low-pass** (anti-alias / reconstruction):
   R6 → R7 in series to +in, C6 1nF from the R6/R7 node to the output, C8 1nF
   from +in to GND, R8 100k biases +in to GND. Equal caps → Q ≈ 0.5.
   fc ≈ 1/(2π·8.2k·1nF) **≈ 19.4 kHz**. R6/R7 = 8.2k is intentional (an older
   BOM said 5.6k, which would put fc ≈ 28 kHz — that was the error).
4. **C5 100nF + R9 330k**: DC-blocking high-pass, ≈4.8 Hz.
5. **U6B: non-inverting gain of 1 + R11/R10 = 1 + 120k/10k = 13**.
6. **R13 1k** series output resistor → J5 tip. J5 pin 2 is unconnected.

## Known discrepancies / gotchas

- BOM lists `SW_DIP_x01` as SW1's value but the footprint is the E-Switch
  KS01Q01 pushbutton — it's a momentary button, not a DIP switch.
- C1's value is typed `100nf` (lowercase) in the schematic — cosmetic only.
- Firmware comments reference old Teensy 3.2 pins and an earlier PCB spin
  ("was 6 before PCB change"); the table above reflects the current board.
- Teensy 4.0 ADC is 10-bit effective vs. 13-bit on the old Teensy 3.2 build;
  ported Radio Music code that assumes 0–8191 must be rescaled.
- Not compatible with Teensy 3.x alternative firmware (e.g. Chord Organ).

## PCB

- Single 2-layer board, 1.6 mm, **20.0 × 107.0 mm** (4HP). Revision silkscreen: `V2.01.00`.
- GND pours on both F.Cu and B.Cu; a **5.0V pour on B.Cu**. Track widths 0.25 / 0.5 / 0.75 mm, 14 vias.
- **Front (panel side, top→bottom):** 4 bank LEDs → RV1 Station pot → RV2 Start
  pot → Reset LED → microSD socket + Reset button → jacks
  (row 1: J1 Station CV left, J6 Start CV right; row 2: J2 Reset CV left, J5 Output right).
- **Back:** Teensy 4.0 (U1), all SMD parts (0603 passives, SOIC-8 ICs, SOT-223
  regulator), Eurorack power header J4 near the lower-middle, fuses/diodes just below it.
- The PCB doubles as the mechanical carrier for panel components; jack and pot
  footprints are custom (`PJ301M-12_Compact`, `Alpha9mmPot`) — don't swap them
  for stock KiCad footprints without checking panel hole alignment.

## License

CC-BY-SA 3.0, derived from Radio Music by Tom Whitwell. Teensy is not covered.
