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
| `amiga/cfg/` | FS-UAE configs (A500 + AROS by default) |
| `amiga/hd/` | Directory mounted as the Amiga's boot drive (DH0:) |
| `tools/emu.sh` | Headless emulator driver (Xvfb + xdotool + ImageMagick) |
| `tools/setup-ubuntu.sh` | Installs emulators and build deps |

All sources are git submodules pinned to the commit that was evaluated; each keeps its own licence.

## Setup

```bash
git clone --recurse-submodules --shallow-submodules <this repo>
tools/setup-ubuntu.sh
```

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
