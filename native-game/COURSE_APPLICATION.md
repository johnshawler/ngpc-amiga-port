# Native course startup and input-driven selector

This checkpoint connects native background initialization, palette preparation,
fade-in and ordered course-selector updates. It is new recovery work, not a
restored copy of the lost runtime. The bounded application accepts button samples
and borrowed assets, produces tile/map/palette state, and reaches the original
accept/cancel result. It is not yet a user-playable Amiga game.

## Native modules

- `course_initialization.c`: main color-mode initializer at original 0x20921e.
  It queues both map clears, resets owned display state, clears all 64 hardware
  sprite records/palette indices, initializes selection/cached selection, queues
  palette jobs and both backgrounds, configures/registers the cursor object,
  then queues the selected course label. Existing service slots, staging colors
  and pending copy descriptors are preserved. A nonzero entry guard returns an
  explicit alternate-path request; that branch is not treated as initialized.
- `course_palette.c`: bounded color-mode group and single-color service jobs.
  It uses one shared eight-slot pool for initialization and final acceptance.
  The tail sorts slots when required, resolves jobs into staging/copy records,
  and frees processed slots. The next head uploads palette words and pending
  background/window controls. An empty copy queue does not write pending controls.
- `course_driver.c`: owns the bounded parent/selector/renderer task ring and
  adapts the previously verified routines. It handles input, seven-step fade-in,
  navigation, direct render requests, sounds as events, randomized selection,
  parent timers, course setup, acceptance/cancellation and task removal.
- `course_application.c`: binds those modules into one application state with
  actual native tile/map/palette arrays and a compositor scene adapter. It does
  not duplicate the service pool: command 84 submits to the same palette manager
  used by initialization and both service-pump boundaries.

All graphics and palette sources are caller-resolved bounded spans. No original
asset bytes, cartridge pointer dereferences or CPU interpreter are in the native
modules. The two backgrounds use the already verified normal 16-bit map loader.
The actual original assets are loaded privately only by reference checks.

## Scheduling is part of correctness

The original scheduler does not return once per frame. Its sentinel waits for
the source update boundary, samples input, uploads staged colors, drains old fill
and stamp jobs, then dispatches the outer owner, parent, selector and renderer.
Its tail prepares service jobs before returning to the sentinel. A removed task
changes the dispatch cursor, but does not cause its parent to run again immediately.

The bounded native update preserves the relevant order:

1. Update the sixteen-byte button state.
2. Upload pending colors/controls.
3. Drain prior rectangle fills, then prior asset stamps.
4. Traverse the parent and any children, once, following updated links.
5. Prepare newly queued palette work.

New drawing therefore becomes visible on the next update. A confirmation sets
its parent's result in the selector; the parent handles it on the next traversal.
The final fade-in callback creates both children, and they run during that same
traversal. These details are verified rather than inferred from a synthetic loop.

Observed original input cadence is one update per two reference display frames:
30.125 logical updates/second at the reference core's reported 60.25 Hz. This does
not establish physical NGPC timing or a game redraw rate. The native application
accepts explicit updates; it does not equate them with PAL 50 Hz, choose a pacing
conversion, or implement an Amiga timer/input-device backend.

## Verification

Final reproducible original/native comparisons pass:

- **768 initializer pipelines:** all 256 fill-slot masks and all three valid
  courses; varying stamp occupancy, service-pool occupancy and prior memory.
  Checks include unrelated non-stack RAM, globals, object registration, both
  drawing queues, cleared sprites and all 12 KiB of tile/map output.
- **1,024 palette pipelines:** 2,048 tail/head rounds and 339 full-pool returns.
  Shuffled slot identities, supported old jobs, stale descriptors, existing copy
  jobs, staging, all 512 palette bytes and pending controls are compared.
- **96 complete selector flows / 7,168 logical updates:** actual background and
  palette initialization, intro, movement/release samples, ordinary accept,
  cancel and modified/random accept. Each update compares task identities/links,
  timer/substate fields, control bytes, selection/cache/cursor, parent globals,
  random cursor, viewport, sound requests, drawing queues, all tiles/maps, and
  palette staging/upload state. Final acceptance uses the shared service pool.

The driver oracle deliberately stubs the original sound body and omits object
realization on both sides. It is not a whole-game video/audio equivalence test.
The separate live scheduler investigation checked 1,160 frames against the
existing reference output with an identical repeat; it was read-only observation,
not execution of the native application inside the original game.

Cartridge-free tests also cover initialization, delayed drawing, same-traversal
child creation, navigation, full exit flows, and the combined application with
composed pixel checks. Host, ASan/UBSan and QEMU m68020 tests pass. The new modules
compile freestanding for m68020 without libc/compiler helper dependencies.
This is functional CPU-target coverage, not new stock AGA frame-time evidence.
Previous rendering measurements retain their original scope. No new game FPS,
full-speed or physical-hardware claim is made here.

## Remaining boundaries

The cursor object is configured and registered, but its object-animation/sprite
realization consumer is still absent. Sound requests exist without a native sound
engine. There is no OS input/display/pacing frontend, complete game startup,
later course gameplay or shot physics. The application is a testable native
course-selector component, not a downloadable playable golf build.

Only valid course selections and bounded color jobs are supported. The palette
queue allows eight copy descriptors and rejects unsupported jobs/overflow before
processing a tail. Asset data must remain valid until queued work is consumed.
Initialization must not silently reset preexisting shared palette state. Callers
must initialize fresh-manager free-slot/pending sentinels explicitly and supply
valid preserved controls, scrolling and viewport state. Node storage is owned
by the driver; background resources and the random cursor remain borrowed.

Errors do not authorize restarting a partially progressed update blindly.
`start` returning the guard-bypass result requires the caller to handle the
alternate parent continuation. Native scene palette inversion remains the
presentation adapter's responsibility, as with the existing renderer.

The next useful integration is native cursor/object realization and an Amiga
input/display frontend for this bounded selector, followed by later game flow
and audio. Rendering cost remains an independent blocker to full-speed gameplay.

## Reproduce

With the authorized private cartridge in the existing local input path:

```
bash tools/check-course-initialization.sh
bash tools/check-course-screen.sh
make -C native-game initialization initialization-sanitize initialization-m68020
make -C native-game driver driver-sanitize driver-m68020
make -C native-game application application-sanitize application-m68020
python3 -m unittest discover -s tests -v
```

The original checks build the pinned reference with isolated test controls and
compile current native source. Test-only state adapters live under `reference/`;
all results, original memory and asset data stay under ignored `build/` paths.
Public CI builds the probe and runs cartridge-free tests, never commercial assets.
