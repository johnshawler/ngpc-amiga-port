# Opt-in scheduled power and accuracy application

`shot_power_application.c` extends the existing first-fire application through
real power and accuracy meters, power-stage cancellation/reentry, and the first
part of first-fire cancellation. It is a separate opt-in source entry point.
Existing application files, target frontends, generated assets and preserved
Amiga disks are unchanged.

Enable opening first, then call `ntm_shot_power_application_enable` instead of
control/fire enable. Use `ntm_shot_power_application_update` afterward. The
adapter binds its callback identities through the existing fire state and uses
the unchanged fire update loop: real input, queued drawing head, world input,
ordered task traversal, graphics/palette tail and eleven-dispatch guard.

## Genuine next stages

The fire gate continues into the actual `209a0a` power parent. It restores the
saved HUD and allocates the genuine `20b70b` child. The child initializes real
speed/limit tables and nine privately resolved glyphs, then runs its original
oscillator, hold behavior, meter tile pairs and marker position. The existing
marker child retires through its real lifecycle-2 path. Spin and club behavior
retain their original lifecycle gates. Scenery drawing/animation continues on
its own task.

The next button edge sets lifecycle 3 and allocates genuine `20bc19`. The spin
and power children retire through their real completion paths; the accuracy
child initializes and advances its own fractional oscillator and marker. The
parent retains both original signed waits and the third-button gate. No child
is fabricated, marked complete early, or bypassed.

The integrated application stops **before `209aa2`**, the actual swing
transition. This is before lifecycle 4, result initialization, swing sound,
trajectory animation, hit/miss publication, or ball flight. The separately
verified accuracy component includes its lifecycle-4 classification and task
completion, but those effects are not claimed for this application boundary.

## Cancellation

Power-stage cancel has priority over a simultaneous power edge. It executes
`209bb0`, the next dispatch's saved-meter restoration `209bc1`, and the following
`209bd6` return to the genuine first-fire wait. The meter retires naturally
when it observes lifecycle 1. First-fire scenery, trajectory and spin are not
reallocated. A subsequent power edge allocates and initializes a new real
meter task. Integration tests exercise two such cycles before leaving.

First-fire cancel executes `209b71`: overlay, viewport closure and actual
higher-priority descendant removal. The next `209b9a` dispatch clears the
original HUD rectangle, resets lifecycle/screen words and computes the camera.
The adapter synchronizes the native camera origin's two views of original
`5710..5713`, then stops **before `2097f0`**. Rebuilding and reopening the aiming
application after this cancellation is not integrated yet.

Both stops retain the published/current task and already-completed update
prefix. Later tasks and the frame tail do not run. Another update is rejected,
so no partially completed frame can be advanced twice.

## Ownership and verification

All state, tasks, video arrays and resources remain native and caller-owned.
The existing twelve-node pool is sufficient; no nodes are added. Glyph assets
are privately resolved borrowed spans, not cartridge addresses dereferenced by
native game code. All 54 existing terrain/feature resource sets remain intact.
The m68020 application extension is 88 bytes beyond the first-fire application;
this is a structure measurement, not a linked-target Chip-memory estimate.

`tools/check-shot-power-application.sh` runs eight authored enable, scheduler,
cancellation and boundary contracts on host, ASan/UBSan and m68020/QEMU. Its
cartridge-free public unittest tests both normal and private-copy backends.
The component scripts separately verify the parent, power meter and accuracy
child against the unchanged original with complete nonstack RAM/video and
lifecycle checks.

Both normal and private-copy backends pass the final 24 flows: 13,154 update
attempts and 62,840 ordered dispatches per backend. Twenty-one flows enter the
real first-fire scene, covering all three courses and six golfer selections.
Five end after live power-meter windows and a separate genuine head drain;
six reach the swing boundary (covering all six golfers); ten reach the world
reload boundary, including five with two power-cancel/reentry cycles. Three
exit earlier in selection. Sixteen attempts stop at deliberate partial-update
boundaries. The observed maximum is nine dispatches in an update.

The application oracle uses normal input edges from course selection through
all new stages. It compares every ordered dispatch, task header/free/active
link, live callback scratch, input state, shared RNG, sound request order,
object registration and queued resource identity, map/tile/sprite/palette
buffers, viewport and scroll state, and golfer/terrain globals. Separate
head-only drains verify queued drawing after the live-meter windows. No core
instruction, child callback, resource or original wait is replaced or patched.

This is source-level integration, not a new playable Amiga package or speed
claim. The published first-fire target remains unchanged. Stock PAL A1200,
68EC020/AGA, 2 MiB Chip and zero Fast remains the preferred target. New packaging,
actual Chip fit and speed require their own measured build. Ball flight,
audio synthesis and full-speed gameplay remain unfinished.
