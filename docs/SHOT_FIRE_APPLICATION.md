# Opt-in scheduled first-fire application

`shot_fire_application.c` extends the existing opening/control application
through the genuine first-fire scene. It is a separate source entry point;
none of the existing control application, exporter, target executables, or
preserved disks is changed.

## Genuine scheduling

Enable opening first, then call `ntm_shot_fire_application_enable` instead of
control enable. Use `ntm_shot_fire_application_update` for subsequent updates.
The wrapper retains the original input update, queued drawing head, world
scroll/input service, task traversal and graphics/palette tail order.

The ready parent enters `209919` naturally from its fire-button edge. Its
initializer already contains the same-call first decrement, so the application
does not decrement it a second time. It performs the real object removals,
viewport closure, HUD clearing, palette request, and allocations in original
spin/trajectory/golfer order. New children are inserted and dispatched by the
real task list. The camera completes through its actual lifecycle branch.
The marker transitions to its real fire waiting callback, including miniature
refresh; the existing club control stays scheduled.

The children use their complete native implementations:

- `20b604` spin bounds and `20b62d` input, sound requests, redraw, timer and
  genuine saved-map restoration. The input gate is the club restore timer.
- `20b3aa` trajectory initialization and recurring `20b432` label/HUD updates,
  with the genuine tag 27 resource family and dense animation mapping.
- `20a58d` golfer initialization with the current hole's actual large/close
  terrain maps, projection, art and tag 51.
- On the next real dispatch, `20a5aa` draws the real reflected rims and selected
  green, performs feature selection and clipped row copies, optionally registers
  tag 52, adjusts the pin, and saves the HUD. It installs `20a5c0` and performs
  the first scenery-animation tick in that same call. Recurring ticks continue
  on the task list; the earlier drawing child is never skipped.

Power `209a0a` and cancel `209b71` remain explicit, distinct boundaries. The
wrapper stops before either action, retains the current/published task and
already-completed frame prefix, does not run the remaining tasks or frame tail,
and refuses another update after that stop. Simultaneous power/cancel edges
retain cancel priority. Ball flight is not implemented.

## Ownership and bounds

All task nodes, state, video arrays and resource spans are native and
caller-owned. Original resource addresses are not dereferenced by this module.
The resource bundle supplies matched per-hole terrain maps and drawing/feature
resources, plus the actual trajectory, pin and green object families. Assets
remain borrowed through their genuine queued consumers.

Three extra task nodes are appended to the existing free list before the first
update. Allocation failure still uses the genuine fallback behavior; no child
is fabricated or reported complete. There are twelve native nodes including
the sentinel, so the update has an eleven-dispatch guard. This separate entry
point leaves the preserved eight-dispatch control driver unchanged. The added
pool is 66 bytes on m68020 (three 22-byte native task structures), or 120 bytes
on the 64-bit host. The full m68020 application-state extension is 388 bytes,
including that pool. The freestanding GCC m68020 wrapper object has 1,956 text
bytes and no data/BSS. Neither number is a linked-target memory estimate.

## Verification

`tools/check-shot-fire-application.sh` runs 32 authored application contract,
allocation exhaustion/order, partial-update and dispatch-bound cases on the
host, ASan/UBSan, and static m68020/QEMU. The public unittest also runs that
fixture with the ordinary and private asset-copy backends. Authored scheduler
callbacks are test instrumentation only; the original oracle dispatches the
real native children.

The separate application bridge and oracle compare the whole selector-to-fire
flow against the unchanged original core. They check per-update task dispatch
order, every task header and free-list link, live callback scratch, input edges,
shared random state, sound-request order, object registration and queued sprite
resource identity, tile/map/sprite/palette buffers, viewport and scroll state,
queued stamp identities, and the golfer's terrain/projection/feature globals.
The standard 24 flows exercise all three courses and six golfer selections:
21 reach the real fire scene, while three take earlier selector exits. Six stop
after 90 live fire updates and a separate genuine drawing-head drain, five
stop at the power boundary, and ten at cancel (including simultaneous-button
priority). This is 12,003 update attempts and 54,249 ordered task dispatches;
15 attempts stop at a deliberate partial-update boundary. The observed peak
is nine task dispatches in one update, not an estimated count.

Use `NTM_FIRE_INPUT_PATTERN=club_then_spin NTM_FIRE_FLOWS=4` with the same
check script for the additional integrated club/trajectory-change, restore,
spin-change and restore sequence. Each backend passes those four additional
flows with 4,179 update attempts and 16,137 ordered dispatches, including one
live window, one power boundary and two cancel boundaries. Both patterns use
ordinary input edges, not memory patches, shortened waits or fabricated child
completions.

The wrapper's extra nodes are initialized as equivalent original free-list
nodes, not synthetic successful allocations. No original instruction, child,
resource, or core binary is replaced. These integrated flows cover the actual
first-tee scenes. Full 54-hole and all green-choice coverage belongs to the
separate component oracles; it is not claimed for these application flows.

This is source-level game-flow integration, not a new playable Amiga build or
a speed claim. Stock PAL A1200 68EC020 with AGA and 2 MiB Chip/zero Fast remains
the preferred target. Resource packing, disk capacity, target integration,
actual Chip-memory fit and stock timing require a separate measured build.
Any maximum-8-MiB-Fast fallback must be labeled separately.
