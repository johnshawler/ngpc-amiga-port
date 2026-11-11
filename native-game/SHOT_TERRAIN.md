# Native terrain lookup

`shot_terrain.c` converts original 0x20fb18, tee-centre test 0x2050f3,
row/run lookup 0x201782 and per-pixel classifier 0x20fbbd. It is a bounded
native helper, not a game loop or an interpreter.

The caller borrows the existing 256-byte hole state and two resolved map spans.
Each map retains its little-endian row-offset table and three-byte run records:
count, classification byte, pattern index. Coordinates select eight-pixel cells.
A run count of zero does not advance the column, but still advances the bounded
stream. Missing rows or exhausted streams fail without modifying the result.
The original eight horizontal masks and 2,048 pattern-row bytes are separate
borrowed spans, not embedded game assets.

Normal mode returns terrain 5 at the original wrapped extent boundaries. It
stores point scratch, then returns terrain 4 inside the inclusive tee rectangle
of ten horizontal and seven vertical units. These early branches preserve the
original terrain-detail word; the extent branch also preserves point scratch.
The close map is selected only inside the original strict-origin/inclusive-55
rectangle, with wrapped doubling and offsets 16/8. Player status 255 selects
that transformed close map unconditionally. The classifier preserves both
packed terrain nibbles and the special rotated-byte detail arithmetic.
All arithmetic is defined with unsigned wrap and explicit sign-bit tests.

The native result includes the two point scratch words, detail and terrain.
Callers must preserve this state between queries. Invalid resources return -1
without mutation; they are not silently interpreted as grass or out of bounds.
Only resources needed by the taken branch are required. In particular, a tee
centre query can legitimately finish before any map is read.

## Verification

- 5,618 full original queries compare all non-stack RAM, including early exits,
  large and close maps, boundary coordinates, four status values and random points.
- 81,920 independent original classifier cases cover every classification byte,
  representative pattern indices and every pixel of an eight-by-eight cell.
- 4,194,304 synthetic tile/pixel combinations cover every classification/pattern
  byte pair and pixel, plus malformed spans and unchanged early fields.
- Host, ASan/UBSan and static m68020/QEMU tests pass.

Run `bash tools/check-shot-terrain.sh` with the existing compiler and QEMU on
PATH. The original comparisons need the authorized local cartridge and existing
reference core. Public CI runs only authored synthetic fixtures.

This does not yet prove the shot application, map resource setup, landscape
rendering, aiming or ball simulation. The prior overview target remains separate.
