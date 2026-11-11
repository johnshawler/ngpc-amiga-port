# Native reflected scenery stamps

`course_screen.c` now translates the reflected branch of original `200581`,
used by the real golfer's immediate scenery pair in `20ab15`. The existing
`20054f` FIFO drain uses the same validated implementation.

Both byte and word sentinel rows use the first row's width as their shared
reflection anchor. Subsequent rows can have different lengths. Tile bit 15
reverses destination traversal and XORs bit 15 of the output word after
16-bit map-word addition. Tile bit 14 is discarded. Tile pixels are copied
unchanged; reflection belongs to the map attributes, not the pixel payload.

The public read-only `ntm_course_screen_stamp_valid` returns zero for a valid
borrowed span, minus one for invalid input. It enables composed callers to
validate all immediate requests before any scene mutation. Each standalone
stamp and the complete FIFO queue retain prevalidation before writes. Invalid
source lengths, empty rows, missing sentinels, reflected negative destinations,
and destination/tile bounds are rejected without mutation.

## Verification

Run `tools/check-course-screen-reflection.sh` with the m68k cross compiler and
QEMU on PATH, the existing reference build prerequisites, and the private
user-supplied cartridge. Nothing from the cartridge is embedded in these tests.

For each default/private-copy backend:

- 4,096 authored byte/word cases covering all flag combinations, variable row
  lengths, and carry-sensitive map values, on host, ASan/UBSan and static
  m68020/QEMU. Separate boundary and atomic rejection assertions also run.
- 512 synthetic unchanged-original `200581` comparisons.
- 32 genuine immediate `20ab15` scenery pairs and 32 FIFO `20054f` drains using
  those same private assets and destinations.
- Each oracle case checks all nonstack game RAM and the complete `8000:bfff`
  hardware/video span. Only the original's measured stack writes are excluded.

The private-copy tests use a memcpy contract fixture, not Amiga assembly DMA
or blitter execution. Existing course-screen normal stamping, producer and
pipeline regression suites run afterward. No ROM/core patches or target
artifacts are made. This is routine-level conversion evidence, not an Amiga
frame-rate, full-game, or target deployment claim.
