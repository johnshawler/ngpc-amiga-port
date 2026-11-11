# Native cursor and selector integration

This recovery implementation adds the original course selector's bounded cursor
object consumer. It does not restore the lost historical runtime and is not a
complete golf game.

`course_cursor.c` implements normal/accept animation, signed position offsets,
clipping, horizontal/vertical flips, hide/pause/restart, forced tile refresh,
queue-full dropping, removal, and double-buffered sprite realization. Resources
are caller-resolved, bounds-checked spans; no original CPU instructions execute.
The supported object is the single registered course cursor, with one component
per frame and color sprites. Unsupported raster/multiplex and collision modes
are rejected, rather than treated as implemented.

The application preserves the original service ordering: palette, fill and stamp
queues drain before cursor upload; task traversal runs next; palette and cursor
preparation run at the tail. Preparation does not immediately change visible
sprite/tile memory. Shared display-reset storage and delayed removals retain the
original behavior.

## Verification

`bash tools/check-course-cursor.sh` builds isolated reference/native comparison
libraries and first repeats initialization regressions. With the user's local
cartridge, the authored harness passes:

- 128 cursor flows, 6,528 preparation/upload rounds.
- 130 queue, pause and start-offset cases, 390 rounds, including all queue
  occupancies from empty through full.
- 96 complete bounded application flows, 7,168 logical updates, comparing task
  state, input, tiles, maps, sprites, palettes, queues, viewport and sound events.

These are comparisons against original execution, not only a second native
model. Original snapshots, asset bytes, trace output and their hashes remain
ignored private artifacts. Public CI does not require the cartridge.

Host, sanitizer and m68020 execution checks:

```
make -C native-game cursor cursor-sanitize cursor-m68020
make -C native-game application application-sanitize application-m68020
python3 -m unittest discover -s tests -v
```

The [Amiga selector frontend](../native-renderer/amiga/selector_README.md)
connects these routines to hardware joystick input and AGA display. It remains a
silent selector-only demonstration. Full golf gameplay and audio synthesis are
not converted; component correctness and timing do not establish full-game FPS.
