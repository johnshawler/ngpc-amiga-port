# First shot preparation and tee-shot banner

This opt-in native continuation implements the original `2096a4` preparation
and its actual `20bf8e` first-tee banner child. It runs after the verified hole
overview. It stops at `209751`, before the next scene/aiming setup. It does not
implement aiming, a swing, ball physics, later lies, or a playable hole.

## Converted behavior

- `20d774`, `20d9fc`, `20d87b`: normal player selection, signed trait-table
  indexing, current/previous coordinates, and shot scratch initialization.
- `20d024`, `20d179`, `20d282`: the game's fixed-point distance approximation,
  multiply and divide, including wrapped intermediate arithmetic. These are
  semantic C implementations, not a CPU interpreter or floating-point substitute.
- `20fb18`: the separately verified terrain helper. The initial shot is at the
  prepared tee centre and returns terrain 4 before accessing either map. No
  invented large-map pointer is installed while the original pointer is unset.
- `20d8b4`, `20500e`, `20506a`: actual stroke-counter updates and two draws from
  the existing shared RNG. The normal wind-limit helper changes the table used
  by the later direction lookup; the native implementation preserves this.
- `20bf8e` through `20c199`: queued terrain, golfer, player and tee-shot stamps,
  original two-update opening delay, blinking prompt, 150-update hold, real
  nested children, skip handling and final cleanup.
- `20c3eb`, `20c45b`, `20c503`: the genuine first-tee notice return, tag-24/index-56
  animation object, completion wait, portrait stamp and idle restart.
- `20c564`, `20c5b9`: 20-update exit, original wrapping column clears, restart
  velocity 156, actual animated palette requests 6c/70, raster installation and
  removal. See [SHOT_PALETTE.md](SHOT_PALETTE.md) and
  [SHOT_RASTER.md](SHOT_RASTER.md) for the separately verified services.

The banner stamps use the deferred map queue, not immediate rendering. Queue
saturation drops a request as in the original. Object and palette service order
is preserved. Sound calls are observable requests only; no audio synthesis is
claimed.

## Application contract

`ntm_shot_application_start` accepts the existing selector/title/round/tee/hole
resources and a borrowed `ntm_shot_resources`. All resource spans must remain
alive until the run finishes. Zero-initialize application state and initialize
the same caller-owned selector/palette state as the existing tee application.
If carrying existing timer state, initialize the owned raster fields explicitly;
fresh live original TRUN is 136 (0x88) and GPU control is 128 (0x80).
Installation preserves
its upper bits. The original comparison covers varied matching timer/shadow
initial states rather than silently normalizing them.
Call `ntm_shot_application_update` with sampled input at the source logical
cadence. A return value of 1 means the run reached a declared boundary, not that
a shot was played. Inspect `application.boundary`:

- `NTM_SHOT_BOUNDARY_SCENE_SETUP`: banner finished and its parent resumed at
  209751. The next setup remains unconverted.
- `NTM_SHOT_BOUNDARY_UNSUPPORTED`: an unconverted branch was requested.

The original five-node driver pool is extended by four real application-owned
task nodes before its first traversal. Allocation order, free-list links,
parent/child identities and finish behavior are compared independently against
original task addresses. No fabricated completion, skipped child or shared
fallback is used to make the banner progress.

The only old-runtime changes are optional `shot_continuation` and palette-tail
hooks in `tee_setup_application`. NULL retains the earlier tee boundary and
palette service. Existing selector/title/round/tee targets remain separate.

The bounded branch is a fresh normal first tee: terrain 4, stroke 1, modifier 0,
player identity 0 or 1, golfer 0 through 5. Skipping the entire overview without
prepared hole state is rejected. Input 64's optional banner menu stops before
its unconverted action. Other lies and alternate game modes are not silently
mapped onto this branch.

## Raster timing boundary

The application owns the exact nine-byte raster service state but does not
invent scanline progression during a logical scheduler update. A target must
advance raw/latched scroll with `ntm_shot_raster_step`, retain the resulting
152-row plan in its display snapshot, and include that plan in dirty detection.
The helper's steady row boundary, first-install latch and blanking behavior have
independent original timer/IRQ tests. The integrated scheduler comparison does
not establish the full live game's scheduler-to-scanline phase. A target timing
policy is not evidence of exact original first-row parity.

## Verification

Public cartridge-free tests:

```
make -C native-game shot-setup shot-setup-sanitize shot-setup-m68020
python3 -m unittest tests.test_shot_setup
```

These cover 100,010 arithmetic vectors, 768 exact geometry states, 131,072 exact
stroke states, 65,536 exact wind states, and 24 authored application flows.
The application fixture covers normal/random selection, cancel, repeated skips,
held input, both active/inactive banner paths, explicit optional-menu and
unprepared-hole boundaries, real task storage and completed palette/raster
lifecycles. Host, ASan/UBSan and m68020/QEMU builds pass. Runtime arithmetic uses
only 32-bit operations; 64-bit arithmetic appears only in the independent test
reference.

Private original comparison, requiring the caller's local ROM and existing
isolated reference core:

```
tools/check-shot-setup.sh
```

- Arithmetic: 2,144 divides, 2,144 multiplies and 3,144 distances.
- Preparation: 1,536 geometry, 2,560 stroke and 17,408 wind cases, comparing
  complete nonstack RAM with real prerequisite calls and shared RNG.
- Banner: 108 initializations, 112 later callbacks, 6 animation callbacks,
  189 exit callbacks and 2 notice returns. This isolated suite compares modeled
  RAM/maps/tiles/objects/tasks and real raster install/remove; it observes the
  two animated-palette enqueue requests at their explicit service boundary.
- Integrated application: 24 flows and 7,410 updates, covering all three
  courses, all six golfers, both player identities, random selection, skips and
  cancel. Real palette/object services and all nested children execute. Task
  identities/free links, input/RNG/player/hole state, maps, tiles, sprites,
  palettes and raster registration are compared after each update. Audio is
  request-only. Raster row progression has its separate original oracle.

The script checks both ordinary and private Amiga asset-copy backends, without
rebuilding or patching the original core. No ROM, extracted resources, captures
or private hashes belong in version control. Resource resolution lives in
`reference/shot_application_case_common.py`; it embeds no extracted bytes.

This source checkpoint does not establish A1200 display timing, memory/disk
fit, or game speed. Those require the separately built and measured target.
