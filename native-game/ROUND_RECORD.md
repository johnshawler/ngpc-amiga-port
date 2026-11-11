# Native normal round-record lifecycle

`round_record.c` converts the original child 0x21480a, its drawing child
0x214941 and its blinking prompt child 0x214c10. The opt-in
`ntm_course_intro_application_start_round_record` continues the existing
selector, scenery, course title and tournament lifecycle through this screen.
The outer owner subsequently reaches an explicit tee-shot request at 0x203f79,
before allocating 0x208753. Tee-shot setup and playable golf are not implemented.
The existing scenery-only and title-only application starts keep their boundaries.

## Supported state and resources

This is the normal single-player fresh-record path: game mode 0, alternate
record mode 0, secondary mode 0, normal golfer record kind 0, missing score 255,
all six golfer indices and all three courses. The application rejects unsupported
variants at an explicit boundary rather than silently drawing a normal record.
There is no restored save progression. The normal preparation helper is described
in [ROUND_PREPARE.md](ROUND_PREPARE.md); it uses the selector's shared random
cursor and privately resolved table, including duplicate-hole retries.

All art, palette jobs, object data and the missing-score string are borrowed,
caller-resolved spans. The native target does not read a cartridge or execute
original instructions. The optional private exporter supplies those spans.
Resources must stay alive through every deferred drawing and object upload.

## Exact scheduling and services

The owner allocates round-record on its own traversal after title cleanup. The
new child initializes later in that traversal. It issues sound-control request
65533, invokes preparation, marks the first player active and second inactive,
resets owned display state, zeros scroll, submits two palette jobs and draws the
font directly. It allocates a drawing child and returns. This child directly
clears map 0's 20 by 19 cells, queues a header and two course stamps, and draws
the footer directly. The font is outside that clear, starting at row 19.

On the next traversal the parent allocates the blinking-score child. Its first
call copies the missing-score glyphs from the *live font map*, after that frame's
drawing head, and stores delay 30 with immediate decrement to 29. The drawing
child queues the golfer and record labels, then removes itself. Queued stamps
retain FIFO order and full queues drop requests individually. Font/footer direct
writes are never dropped just because the deferred stamp queue is full.

On the following parent callback, requests 65535 and 65534 precede opening the
160 by 152 viewport. Delay 239 is stored without decrement. Later callbacks use
the original pressed-input skip helper or decrement the 16-bit timer modulo
65536 and finish when its result is negative. Ordinary expiry therefore takes
240 more parent visits. Skip preserves the timer. Cleanup closes the viewport,
resets owned display fields, removes descendants, and removes/recycles the
parent task. The outer owner resumes on its next scheduled visit and stops at
the tee-shot boundary. Stable completion does not repeat cleanup.

The prompt's initial delay creates tag 33, resource index 32, at (80, 28), tile
464, registered through the existing object list. Its duration-2 frame reaches
a stop header and freezes on the current frame. Every later visibility transition
stores 5 and immediately decrements to 4, giving five subsequent visits between
toggles. The same native object service now supports this bounded format, with
clipping, flips, freeze/restart, removal and deferred double-buffered uploads.
Display reset preserves unowned prior title-object fields until initialization.
The rendering adapter still supports one active object only; special record
objects and multiple players are outside this path.

The shared stamp consumer now supports both byte and word sentinel rows in
normal orientation. It validates each complete queue before writes, including
truncated rows, empty rows, tile bounds and final map-word boundaries.

## Verification

- Cartridge-free tests exhaust all 262,144 timer states across the parent and
  three blinking callbacks. Additional cases cover direct/deferred stamps,
  preparation/request ordering, cleanup, descendants and unsupported inputs.
- The prompt-object synthetic test covers all 65,536 flag/delay pairs. Its
  original oracle covers 256 cases / 2,048 tail-head rounds. Narrow
  stamp tests cover truncation, missing/empty rows, alignment and map limits.
- The isolated original oracle covers 683 cases: 297 initializations, 90
  drawing cases, 168 later callbacks and 128 blink callbacks. It compares full
  modeled non-stack RAM, direct
  font/footer writes, palette/stamp saturation, exact ordered service calls,
  drawing children, score glyph copying, cleanup and blink registration.
- The integrated original comparison covers 24 flows / 4,419 updates through
  all three courses, six golfer indices, normal/modified accepts, cancellations,
  timed and skipped rounds. It checks task links, callbacks, free pool, record
  preparation and shared RNG, objects, controls, tiles, maps, sprites, palettes,
  drawing queues, viewport and sound-control requests after every update.
- Generic and private-copy backends use the same original oracle. Host,
  ASan/UBSan and QEMU m68020 tests exercise the authored native code.

Sound requests are compared, not audio synthesis. Original-core noninterference
and raster checks are separate from the native conversion. Target performance
and stock PAL A1200 display evidence belong in the separate target report.
None of these bounded checks establish whole-game speed or playable golf.

```
make -C native-game round-record round-record-sanitize round-record-m68020
python3 -m unittest discover -s tests -p 'test_round*.py' -v
bash tools/check-round-record.sh
```

The private oracle uses an existing screen-probe core and the user's cartridge.
Do not rebuild that core while any reference process is running. All cartridge
bytes, derived assets, captures and generated reports stay under ignored paths.
