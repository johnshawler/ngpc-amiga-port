# Native course-screen drawing

Later integration: [COURSE_APPLICATION.md](COURSE_APPLICATION.md) adds native
background/palette initialization and an input-driven selector task loop. The
older checkpoint scope and measurements below remain historical.

This is new recovery/reimplementation work, not restoration of the lost runtime.
`course_screen.c` converts the actual course-selection label drawing service
(0x209603), its change-gated task (0x2095e3), and the bounded drawing-queue
consumers required by its three valid course assets. It contains no game asset
bytes, cartridge pointer dereferences, CPU interpreter or allocation.

## What now runs natively

The renderer task compares the full 16-bit selection against its cached word.
A change updates that cache before producing drawing requests. Direct parent
render requests do not update the cache. Only selections 0..2 are supported;
invalid selections are rejected rather than interpreting unrelated asset bytes.

Drawing stores the selected course and cursor position, then queues two jobs:

1. Clear a 20-by-6 tilemap rectangle at byte offset 0x300. Fill allocation scans
   free status 255 slots from 7 down to 0; a full queue silently drops this job.
2. Load the selected label's tiles starting at tile 301, then stamp its map at
   byte offset 0x302+12*selection. The 32-entry stamp queue is FIFO; a full queue
   silently drops this job independently of the fill queue.

Neither producer writes tile/map arrays immediately. The original frame calls
fill consumption before stamp consumption (call sites 0x200150/0x200154).
`ntm_course_screen_fill_queue` and `ntm_course_screen_stamp_queue` preserve these
separate scheduling boundaries. Fill consumers scan 7 down to 0, freeing each
slot before its work; every status other than 255 is active. Stamp consumption
clears its byte-count after draining, while preserving old records.

The source asset format supplies tile count, tile-data pointer, tile offsets,
then sentinel-terminated map rows. The native loader accepts caller-resolved,
bounded data/tile spans; it never dereferences the embedded original pointer.
Its supported course path has normal orientation and 16-bit map cells. Each
cell adds the tile/palette base with 16-bit wrapping. Tile pixels and map bytes
are written directly into arrays usable by the existing native compositor.

`ntm_course_parent_render` now fulfills a pending parent render request using
this producer instead of requiring a drawing stub. It resumes the parent only
after producing the jobs; the caller must still drain shared queues at their
scheduled boundary. Sound remains a separate, unconverted service.

## Verification

Final authored source passes:

- 196,608 original/native gate comparisons: all cache words for each valid course.
- 25,344 original/native producer comparisons: three courses, all 256 free-slot
 masks, all 33 stamp-queue occupancies.
- 271 immediate original/native stamp comparisons: all three actual assets under
 five initial scenes each, plus 256 synthetic nonempty variable-width maps.
- 128 complete mixed-queue original/native pipelines containing 3,216 gated
 selection requests. Queue metadata, drop behavior and all 12 KiB of tile/map
 memory agree after fill and stamp consumption.
- 221,952 independent native producer/cache cases, synthetic ordering and malformed
 input checks, on host, ASan/UBSan and QEMU m68020.
- 1,536 complete native parent flows now use real drawing producers/consumers
 rather than render stubs, with an authored label asset. Their composed row and
 eight-plane output are checked pixel/bit-wise. Host, sanitizer and m68020 pass.

The existing 85,088 service comparisons and 1400-frame navigation reference run
remain unchanged, including video, audio and final state, with a fresh repeat.
That live run is a regression check: it does not replace the original live
screen routines. The new screen comparisons above are isolated routine and
queue-pipeline tests. The parent flow comparison still stubs original render
calls; the separate screen comparisons verify the converted service itself.

The final source builds to a freestanding m68020 object with no unresolved
symbols and links into the existing parent fixture. There is no new stock-target
speed measurement in this checkpoint. Previous full-composition costs remain a
known problem, not a claim of game FPS or playable golf.

## Contracts and remaining work

Inputs and output arrays are caller-owned and disjoint. Borrowed asset spans
must remain valid until queued work is consumed. Native destinations are the
8192-byte tile array and 4096-byte pair of tilemaps. Cache invalidation/latching
is the caller's responsibility after these writes.

Malformed/unsupported jobs are rejected before that queue mutates: zero tile
count, zero rows, empty rows, zero fill dimensions, mirrored or 8-bit maps,
out-of-bounds spans/destinations or invalid queue counts. The original has
wraparound/do-while behavior for malformed zero cases; no equivalence is claimed
for those cases. These restrictions cover the three actual course-label assets,
not every drawing job in the game.

A complete native frame scheduler, initial screen/background loading, palette
updates, joystick-driven application, audio, later course flow and shot physics
remain unfinished. This checkpoint advances actual drawing but is not a playable
port. A useful next gate is a bounded native course-selector loop with the real
background/label assets and original scheduling boundaries.

## Reproduce

Run from the repository root with the privately supplied game in its documented
local input path:

```
bash tools/check-course-screen.sh
make -C native-game screen screen-sanitize screen-m68020
make -C native-game services services-sanitize services-m68020
python3 -m unittest discover -s tests -v
```

The first command builds the pinned original core with isolated routine controls,
compiles the current native module and runs all three original comparisons.
Its outputs stay under ignored build/course-screen-check. No cartridge, private
trace, asset data or firmware is included in the repository. Public CI compiles
the probe and runs cartridge-free tests only.
