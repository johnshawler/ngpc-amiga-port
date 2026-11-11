# Active aim camera and incremental world scrolling

`shot_camera.c` translates original `20c8a5`, its `20ca34` object-positioning
helpers, and the main-loop world-scrolling service `201608`. It does not start
a swing, simulate the ball, or replace any pending work with a success stub.
The optional aiming application must explicitly dispatch the active camera and
call the scrolling service at the real main-loop location.

## Camera behavior

- The existing `shot_markers.c` camera initializer and signed countdown remain
  unchanged. The active callback consumes the active player's genuine pressed
  and held bytes. Button bit 5 enters pan mode; directional bits select the
  original eight 8-pixel pan vectors. Opposing directions do not move.
- State 0 saves the original origin into `saved_x/saved_y` (`4417/4419`). State 2
  uses the exact wrapped arithmetic and signed camera bounds of `204e94`.
  Neutral and contradictory directions still refresh the miniature ball, pin,
  gauges and miniature direction marker without clamping the origin.
- Releasing the pan button sets state 4 and restores the saved origin. If the
  origin changed, the original viewport is closed for that update and the
  miniature objects refresh. The next state-4 callback opens the viewport and
  returns to state 0. An unchanged origin skips closing and object refresh but
  still goes through state 4.
- The object refresh clears the shared dirty byte and updates the actual
  fixed-point direction fields and miniature marker. It does **not** reposition
  the main direction marker, register new objects, or enqueue drawing assets.
- Any nonzero shot lifecycle value performs genuine descendant removal and
  task completion, including active/free-list links. Invalid camera modes are
  explicit unsupported results; no jump-table bytes are treated as code.

The API borrows `ntm_shot_marker_context` and the existing quarter-wave resource.
No new art, cartridge data, heap allocation, floating-point or 64-bit runtime
arithmetic is needed. The two saved-origin words belong to the camera state.

## World service and ordering

Original `201608` runs after the drawing head and before task dispatch when
flag `400d` bit 3 is set. It is **not** called by the active camera callback.
The caller supplies the current aim origin in world-state bytes 0..3 and writes
the returned raw scroll bytes to layer 2 (`8034/8035`). Consequently camera
movement updates the world map on the next main-loop pass, as in the original.

The service uses the existing bounded layout and block windows. It preserves
map identities, tile data, palettes, queues and all unrelated state. Only newly
exposed seven-block rows/columns are copied to the circular map. The positive
threshold is inclusive at 32 pixels; the negative threshold is strictly below
-32. Horizontal movement processes one column per pass. A vertical update
loops back through the horizontal check, preserving the original diagonal and
large-jump behavior. Source/destination validation runs before mutable output
changes, so malformed resource spans cannot leave a partial scroll update.

## Verification

Cartridge-free public fixture:

```
python3 -m unittest tests.test_shot_camera
```

The fixture checks 65,536 pressed/held pairs, 256 restore/unsupported-state cases,
1,020 lifecycle removals, 65,536 authored scroll cases and atomic malformed-span
rejection. It passes natively, with ASan/UBSan, and on m68020/QEMU.

Full private suite with the unchanged original core and the user's existing
local cartridge:

```
source ../amiga-recovery-m68k/activate.sh
tools/check-shot-camera.sh
```

- 3,072 active-camera comparisons cover states 0/2/4, held/pressed input bytes,
  wrapped signed bounds, viewport writes and all refreshed object state.
- 1,020 original task-exit comparisons cover all 255 nonzero lifecycle values,
  zero to three descendants and mixed task-recycling flags.
- 912 incremental scroll comparisons cover all 108 original map records, both
  map layers, exact threshold asymmetry, diagonal and multi-strip movement,
  wrapped negative origins, and signed coordinate/difference extremes.

Camera/scroll comparisons check all nonstack game RAM and the complete hardware
and video range. Exit comparisons check all nonstack RAM and the task cursor.
Only actual original stack scratch is excluded. The tests use no fabricated
queues, masked state differences or patched reference instructions.

These source checks establish exact bounded behavior, not target RAM/disk fit,
stock A1200 display cadence, or full-game speed. No cartridge bytes, extracted
art, captures or private input fingerprints belong in version control.
