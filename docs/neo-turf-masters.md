# Neo Turf Masters (NGPC) — port notes

The ROM is **not** in this repo. Put your own dump at `rom/` (git-ignored).

## Cartridge

| Field | Value |
|---|---|
| File | `Neo Turf Masters (World) (En,Ja).ngc` |
| Size | 2 MiB (16 Mbit); 1,877,906 bytes used before trailing `0xFF` padding |
| SHA-1 | `308361297bd28ebb8fb60713ae5e00cee514415c` |
| Header licence | `LICENSED BY SNK CORPORATION` |
| Entry point | `0x200052` (cart ROM is mapped at `0x200000`) |
| Software ID / version | `0x0035` / 17 |
| System | `0x10` (colour) |
| Internal title | `B_T_GOLF` |

Runs fine in Mednafen's NGP core with its built-in BIOS emulation (no BIOS dump needed).

## Flow observed in the emulator

SNK logo → title ("PRESS BUTTON A") → mode menu (1 Player / VS Mode Play / Game Options)
→ 1P sub-menu (Stroke Play / Handicap / Triple Crown) → Golfer Select (6 golfers, 5 stat bars)
→ Course Select (Japan "Fuji", U.S.A., Germany) → course splash → "Free Round Start"
→ tee shot (save/resume prompt) → overhead aim view → behind-golfer swing view with power meter
→ overhead ball-flight view, hole mini-map on the right.

## Mednafen keys (for `tools/emu.sh key ngpc ...`)

| NGPC | Key |
|---|---|
| D-pad | `w` `a` `s` `d` |
| A | `KP_2` |
| B | `KP_3` |
| Option | `Return` (also soft-resets from title) |

## A1200 port considerations

* NGPC screen is 160×152, 2 scroll planes + 64 sprites of 8×8 tiles, 4 colours per palette
  (3 + transparent), 16 palettes per plane/sprites, 12-bit RGB. AGA lowres 320×256 fits it 2×
  horizontally or 1:1 with a border; 12-bit colours map exactly into AGA's 24-bit palette.
* Each layer (scroll plane 1, scroll plane 2, sprites) has 16 palettes × 3 colours, plus one
  background colour: up to 145 colours on screen → AGA 8 bitplanes (256 colours) covers it
  without reduction, keeping one 64-colour bank per layer for simple palette remapping.
* CPU is a TLCS-900H @ 6.144 MHz with a Z80 sound CPU driving a T6W28 (SN76489-style) PSG plus DAC.
  Options: (a) re-implement game logic in C from a disassembly, (b) static recompilation of
  TLCS-900H → 68020, (c) an NGPC emulator on the A1200 (likely too slow at 14 MHz).
