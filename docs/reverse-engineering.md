# Reverse-engineering toolchain

`tools/analyze.sh` runs the whole pass (≈40 s). Everything it writes goes to `build/`, which is
git-ignored because it is derived from the ROM.

```bash
tools/analyze.sh                       # uses the first rom/*.ngc
```

| Tool | What it does |
|---|---|
| `tools/ngpc-trace/` | Headless NGPC runner: the Mednafen NGP core (from `third_party`, unmodified; one-line coverage hook patched into a copy at build time) driven by an input script. Records every executed instruction address and dumps VRAM/RAM/frames. `NGPC_LANG=english\|japanese` (default english). |
| `tools/ngpc-trace/scripts/ntm-tour.txt` | Boot → menus → first hole, dumping each screen, then 60,000 frames of pseudo-random input. |
| `tools/tlcs900dasm/` | `ngpc-dis`: recursive-descent TLCS-900/H disassembler around MAME's decoder (`vendor/`, BSD-3, one local fix). Follows flow from the entry point, interrupt vectors, validated jump tables and ROM pointers, plus runtime coverage (`--cov`). Writes a labelled listing and a symbol file. |
| `tools/ngpc-gfx/ngpc_vram.py` | Decodes VRAM dumps: palettes, both scroll planes, sprites, the full screen. Recomposes the screen and checks it pixel-for-pixel against the emulator frame. |
| `tools/ngpc-gfx/ngpc_assets.py` | Matches loaded tile runs back to the ROM and exports each graphics block as an indexed 4-colour PNG with the palette it was shown in. |

Outputs: `build/disasm/game.asm` + `game.sym`, `build/gfx/*_{screen,scr1,scr2,tiles}.png` + `.json`,
`build/assets/blk_*.png` + `assets.json` + `rom_coverage.txt`.

## Findings so far (Neo Turf Masters)

### Code
* Entry `0x200052`: kicks the watchdog (`ld (0x6f),0x4e`), picks a start-up routine from the
  table at `0x2002a2` indexed by BIOS state `(0x6f84)`, sets the stack at `0x4000`, and installs
  the VBlank handler `0x2000fb` at vector `0x6fcc`.
* BIOS calls go through one wrapper, `sub_219623`: `RW3` = function number, indirect call through
  the BIOS jump table at `0xfffe00`.
* Main loop `loc_201377` walks a **task list in RAM** (`0x4e86`, current entry at `0x570a`) and
  calls each entry's function pointer. Static tracing cannot see those targets; runtime coverage
  supplies them.
* Current pass: ~17,500 instructions / ~61 KB of code decoded (3.3 % of the 1.8 MB used ROM).
  Every executed address is a decoded instruction start; 25 indirect jumps/calls are still
  unresolved (listed at the end of the listing).
* Coverage varies slightly between runs, most likely because the core takes the console's
  real-time clock from the host clock, so runs are not repeatable.

### Graphics
* **Uncompressed.** In all 14 dumped screens, every non-blank tile in character RAM appears
  byte-for-byte in the ROM. No decompressor is needed for the port.
* The VRAM decoder reproduces all 14 screens pixel-for-pixel, so the tile, map, palette and
  sprite formats (and the layer order) are understood correctly.
* 14 screens locate ≈3,100 tiles (49 KiB) in ≈75 ROM ranges. More screens (other courses,
  golfers, modes, results) are needed to map the rest; add `dump` steps to a trace script.

## Next steps
1. Longer and more targeted trace scripts (all three courses, every golfer, VS mode, options,
   results screens) to grow both code coverage and the graphics map.
2. Find the code that loads tiles/maps/palettes (writes to `0xa000`, `0x9000`/`0x9800`,
   `0x8200`–`0x837f`) to recover the asset tables directly instead of from screenshots.
3. Name the main task functions and RAM variables (`build/disasm/game.sym` is the starting point).
4. Prototype the A1200 renderer: NGPC 2bpp tiles + per-tile palettes → AGA 8-bitplane screen.
