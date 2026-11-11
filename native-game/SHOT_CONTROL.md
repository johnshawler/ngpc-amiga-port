# Native ready, camera and club control

This opt-in application continues the first-open scene without inventing a
completion timer. It enables genuine ready/input, marker, club and camera ticks.
It is not a complete playable hole, swing, ball simulation or audio engine.

## Converted loop

`ntm_shot_control_application` embeds the preserved opening application. Call its
enable function after all existing start/aim/open enables, before any update.
The normal local-player bindings from original `20d68c..20d6c2` are initialized
at that point. Original input identities are retained in player records for
comparison, but native routines consume caller-owned input buffers rather than
dereferencing original-machine addresses. Linked mode is not substituted.

The normal original main-loop order now includes `201608` world scrolling after
old drawing requests and object uploads, before task dispatch, when flag bit 3
is set. This service writes newly exposed map strips and raw layer-2 scroll
bytes. The previous opening target verified a bounded routine replay without
this service. Its exact reference picture is therefore not a reference for the
new, more complete control loop.

Camera task writes update both owned representations of the aliased origin
fields immediately. The scrolling service sees those origins on the next
update, as in the original. It does not reload all world tiles or palettes.

The parent preserves `2098ee`'s signed countdown and same-call transition into
`2098ff`. It waits on the actual camera gate and consumes the active player's
rising-edge fire bit. Held left/right marker movement, edge-triggered club
changes, timed highlights/restoration, camera-button panning and restoration
execute their real native routines. Neutral/contradictory camera directions
still perform the original miniature/gauge/marker refresh.

Club sounds remain distinct service-2 requests `(2,0,0)`, not selector sounds and
not synthesized audio. Marker, camera, club and parent state continue to share
the original dirty, lifecycle and camera flags.

## Explicit boundaries

Fire currently stops immediately before `209919`. The information overlay and
reset dispatch also stop before their unconverted actions. The boundary is a
partial update: the scheduler leaves its current/published task in place,
does not execute later tasks or the tail, and does not increment the completed
update count. No child completion or swing effects are fabricated.

`NTM_TASK_BOUNDARY` and the optional pre-task hook are inactive in existing
applications. With all continuation pointers NULL, the old opening target still
stops at the same first-visible boundary.

## Verification

- `reference/compare_shot_ready.py`: 256 original player-binding comparisons,
  6,360 ready/countdown cases and 5,376 bounded input-admission cases, all with
  full nonstack RAM comparison. Fire/overlay/reset cases stop at exact original
  action boundaries rather than replacing those actions.
- Camera and club component proofs are documented in `SHOT_CAMERA.md` and
  `SHOT_CLUB_CONTROL.md`, including exact map strips, lifecycle, queued drawing
  assets and sound-request order.
- `tools/check-shot-control.sh` compares all three courses and six golfers,
  task/free links, input, shared RNG, player/hole/aim/camera state, maps, tiles,
  sprites, palettes and deferred jobs. Both ordinary and private-asset backends
  pass 24 flows / 10,077 complete updates / 21 separate head drains each.
  Each non-cancel case runs 124 real
  post-opening updates with authored hardware-style button sequences, then the
  test stops observing. The game state is not changed to complete the test.
- `make -C native-game shot-control shot-control-sanitize shot-control-m68020`
  runs the same authored application flows on host, sanitizers and m68020/QEMU.
  Driver fixtures separately check partial-update boundaries and head/task/tail
  ordering. Ready fixtures exhaust all 16-bit countdown values. The complete
  public suite passes 150 tests.

Stock memory, disk fit, pixel equality and performance require the separate
Amiga control frontend gates. Component or host timing is never reported as
whole-game FPS. Original cartridges, extracted assets and captures stay private.
