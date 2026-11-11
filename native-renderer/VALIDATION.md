# Validation checkpoint, 2026-11-10

Scope: new tile graphics primitive and synthetic public fixtures only.

| Check | Result |
| --- | --- |
| Host C99 build, `-O2 -Wall -Wextra -Werror` | Passed |
| Host exhaustive C fixture | Passed |
| Python unittest integration | Passed |
| AddressSanitizer and UndefinedBehaviorSanitizer | Passed, leak detector disabled |
| GCC 14.2.0 `-m68020 -msoft-float -ffreestanding -fno-builtin` object | Passed |
| Undefined symbols in freestanding renderer object | None |
| Renderer object text/data/bss | 830 / 0 / 0 bytes (this toolchain/build) |
| Static m68k/Linux fixture on QEMU user 10.0.13, `-cpu m68020` | Passed |
| Amiga executable / display / actual Chip RAM integration | Not implemented or tested |
| PAL stock A1200 timing / game equivalence / 100% speed | Not tested or claimed |

Each completed exhaustive execution reports:

```
PASS: 131072 row decodes, 65536 descriptors, 4000 viewport draws, 65536 colors, guards and invalid inputs
```

The existing privately extracted Debian cross tools were used, without installing
software or changing global settings. Their activation script in this workspace
is `../amiga-recovery-m68k/activate.sh`; that is a local convenience, not a
repository or CI dependency. Reproduction uses standard cross-tool names on PATH.

A diagnostic QEMU run with `-one-insn-per-tb` was canceled for excessive runtime;
it produced no completed result. The normal explicit `-cpu m68020` run completed
all cases successfully. It is the completed run listed above. QEMU is a CPU
execution check only, and this Linux test binary cannot be launched by AmigaOS.

## Follow-on synthetic Amiga display

The new amiga/ harness now passes actual PAL A1200/AGA emulated display and Chip
memory validation using 2 MiB Chip and zero Fast. All 81,920 planar bytes, all 256
palette colors and every captured pixel including borders match independently
computed synthetic expectations. The repository-owned headless host reproduces
the capture. See amiga/README.md for tool pins, commands and limits.

This supersedes the earlier absence of a synthetic display harness only. It does
not establish game integration, audio, playable controls or rendering speed.
