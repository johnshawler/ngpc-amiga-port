# Synthetic native Amiga display harness

This is new native Amiga code using the tile renderer, not the missing golf runtime.
It is for a disposable emulator session only: after AllocMem it takes over display,
DMA and interrupt hardware and loops until emulator reset. There is no AmigaOS
return or physical-hardware user interface. Never launch in a session with unsaved work.

The executable requests 100,000 bytes of cleared Chip RAM through Exec AllocMem,
renders synthetic tile patterns into eight 320x256 separate bitplanes, and builds
a Copper list with pointers, PAL display/fetch configuration, all 256 palette entries
and both AGA high/low nibble writes. No cartridge data is used.

## Reproduce

Required tools on PATH: m68k-linux-gnu-gcc, m68k-linux-gnu-as, m68k-linux-gnu-ld,
m68k-linux-gnu-nm, m68k-linux-gnu-size; host cc; Python3 with Pillow.

Set AMIGA_BIN to official vscode-amiga-debug/bin/linux containing elf2hunk and
exe2adf, then:
bash native-renderer/amiga/build.sh

Set PUAE_CORE to the built official puae_libretro.so, LIBRETRO_INCLUDE to a directory
containing the official libretro.h, and KICKSTART to a locally authorized A1200
Kickstart 3.1 ROM, then:
bash native-renderer/amiga/run.sh
python3 -m unittest discover -s tests -v

The build creates a relocatable Amiga Hunk executable and bootable ADF under
ignored build/native-display. The run script builds the repository's own headless
host, selects A1200OG / zero Fast, runs 1,200 emulator refreshes, dumps the Chip map
and last RGB image, then compares every displayed pixel and all bitmap bytes.
This frame count includes boot and does not measure rendering speed.

## Verified tools and outcome

- Linux m68k GCC 14.2.0; elf2hunk/exe2adf supplied by vscode-amiga-debug pin
  055097bba74dd1b2f764dcb90781d2017bd1d499.
- Official libretro-uae pin d366f77c60e5a5f059f129cc3773d3048306083b.
- Libretro header from the pinned original NGPC reference submodule.
- Generated emulator config: 68020, AGA, PAL, chipmem_size=4 (2 MiB), zero slow/Fast.
- Own standalone host capture PASS: all 81,920 Chip bitmap bytes, every RGB pixel
  including border, all 256 palette colors with both nibble banks, Chip allocation
  header and completed-fixture marker. Copper list is 2,232 bytes.
- Capture is 720x287; the 320x256 low-resolution viewport is horizontally doubled
  with origin (40,18). Verifier derives translation from bounds and then checks
  the entire image, never crops away discrepancies.
- 13 local repository tests pass, including five deliberate capture/config
  mutations rejected. CI skips the two capture tests unless a local Amiga capture
  exists; firmware is never sent to CI.

An initial development image was black because DIWHIGH omitted the display-window
high bits. The final 0x2100 value explicitly encodes end coordinates 0x1c1/0x12c;
the independent pixel comparison passes. Only final captures are evidence.

## Limits

Static synthetic graphics only. No game renderer ordering, gameplay, raster
effects, audio, interactive controls, throughput or full-speed result. This is
emulated Chip/AGA integration, not physical hardware. Raw Chip dumps can contain
firmware-related data and remain private/ignored. No ROM, screenshot, memory dump,
ADF or generated checksum report is committed with these sources.
