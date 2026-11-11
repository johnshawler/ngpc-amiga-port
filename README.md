# ngpc-amiga-port

Workspace for porting a Neo Geo Pocket Color (NGPC) game to the Commodore Amiga.

## Layout

| Path | What |
|---|---|
| `third_party/ngpc/sodthor-ngpcdev` | NGPC homebrew: LLVM-based TLCS-900 toolchain, transpiler, games with source and built ROMs |
| `third_party/ngpc/nfroggy-ngpcdev` | ASM900 assembly sample + `ngpcspec.txt` hardware spec |
| `third_party/ngpc/NgpCraft_dev_ref` | NGPC hardware registers / CPU / toolchain reference |
| `third_party/ngpc/beetle-ngp-libretro` | Mednafen NGP core (emulator source, hardware behaviour reference) |
| `third_party/ngpc/RACE` | RACE NGPC emulator source |
| `third_party/amiga/amiga-gcc` | bebbo's m68k-amigaos GCC cross toolchain (with vasm, vlink, NDK) |
| `third_party/amiga/vscode-amiga-debug` | BartmanAbyss VS Code Amiga dev/debug/profile extension |
| `third_party/amiga/fs-uae` | FS-UAE Amiga emulator source |
| `amiga/cfg/` | FS-UAE configs (`a1200.fs-uae` is the default; Kickstart from `rom/kick*.rom`, else AROS) |
| `amiga/hd/` | Directory mounted as the Amiga's boot drive (DH0:) |
| `tools/emu.sh` | Headless emulator driver (Xvfb + xdotool + ImageMagick) |
| `tools/setup-ubuntu.sh` | Installs emulators and build deps |
| `src/hello_aga/` | A1200 AGA smoke test (toolchain + emulator check) |
| `docs/neo-turf-masters.md` | Notes on the game being ported |

All sources are git submodules pinned to the commit that was evaluated; each keeps its own licence.

## Setup

```bash
git clone --recurse-submodules --shallow-submodules <this repo>
tools/setup-ubuntu.sh
```

## Amiga toolchain

bebbo's amiga-gcc (GCC 6.5, binutils, gdb, libnix, NDK 3.2, vasm) installs to `/opt/amiga`.
Build it with `make all -k` (see `tools/setup-ubuntu.sh`): `libSDL12` and `libnix4.library` fail
to build with the pinned revision and are not needed.

```bash
make -C src/hello_aga        # builds amiga/hd/hello_aga
tools/emu.sh amiga           # A1200 boots DH0:, S/Startup-Sequence runs hello_aga
```

`hello_aga` opens a 320×256×8 AGA screen and shows a 12-bit colour ramp for 5 s.
Note: booting from a bare DH0: (no Workbench, no SetPatch) leaves Kickstart 3.x's
graphics.library in ECS mode; programs must call `SetChipRev(SETCHIPREV_BEST)` to get AGA.

## Running headless

```bash
tools/emu.sh start-x
tools/emu.sh ngpc third_party/ngpc/sodthor-ngpcdev/PocketRace/POCKETRACE.ngp
tools/emu.sh amiga
tools/emu.sh key amiga d i r Return
tools/emu.sh shot amiga shots/amiga.png
tools/emu.sh stop
```

## Not included

Commercial ROMs, the NGPC BIOS (needed by MAME) and Amiga Kickstart ROMs are copyrighted and must be
supplied separately. `rom/` and `*.rom` are git-ignored.

## Native recovery work

The independent recovery implementation is documented in [native-renderer](native-renderer/README.md) and [native-game](native-game/README.md). The latter now includes verified native background/palette initialization and an input-driven course-selector task loop. The [bounded Amiga selector frontend](native-renderer/amiga/selector_README.md) now connects native cursor animation and hardware input to an actual display. It is a silent selector demonstration, not a playable or full-speed golf game. Commercial assets remain excluded.
