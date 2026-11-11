# Native bounded tee object service

`tee_objects.c` is a separate, cartridge-free implementation of the object
services required by the tee setup screen. It does not widen the existing
selector/title/round prompt engine's contract. Its state and borrowed resources
are supplied by the caller; no cartridge bytes or extracted assets are tracked.
This is a rendering dependency, not playable golf, audio synthesis or a speed
claim for the stock PAL A1200 with 2 MiB Chip RAM and no Fast RAM.

## Supported resources and state

The tee setup registers tags 30, 29 and 31, with resource index 52 and initial
animation offsets 12, 0 and 4. The optional badge is tag 32 with offset 16, 20,
24 or 28; tag 31 can also use offset 8. The separate assets structure contains
eight overview animation spans in a 40-slot array, plus shape and tile spans.
The additional slots support the separately bounded first-tee extension below. Each listed
object can use a different resource structure. Records and their frame, delay
and bounds are indexed by original object IDs 0 through 31.

Supported frames contain one or two noncollision components and color sprites.
The optional badge requires two components for three of its animation choices.
Tag 29 alternates a visible shape with a terminal empty shape. Empty shapes do
not emit sprites or queue tile copies. Stop headers freeze the current frame,
retain delay zero and set the original stop flags; loop headers wrap the frame
and set the loop/change status. Restart, freeze, flips, per-object signed
component offsets and forced refresh are preserved. The original clipping
comparison is inclusive at sprite coordinates x=159 and y=151, and accepts
partially visible coordinates down to -7 with 16-bit wraparound.

Hide is supported for single-component frames. Hidden multicomponent frames
use a different original parser path that skips offset advancement; this
bounded service rejects them rather than silently inventing normal behavior.
The converted tee setup never hides the badge. Collision-linked objects,
other resource indices/tags, more than two components and unsupported badge
animation offsets are also explicit validation failures.

## Registration, ordering and deferred work

`ntm_tee_object_register` appends the ID/priority pair, marks ordering dirty,
sets restart, clears status and stores the no-link sentinel. Full lists return
1 without mutation. Tail first removes marked records by swapping the final
entry into the removed slot and revisiting it. Changed priorities dirty the
list. Dirty lists use stable descending priority order; equal priorities retain
their current order, including any changes caused by removal.

`ntm_tee_object_remove` implements the explicit removal helper separately. It
swaps the final list entry into the removed slot but does not mark ordering
dirty. Repeating removal of an already removed record returns 1. Collision
unlinking is unsupported. `ntm_tee_objects_reset` resets the owned display fields
and synchronizes every record's ID/tag only. Frame, delay, bounds, unrelated
record fields, bank 1 and buffered palettes are deliberately retained.

Tail prepares the selected sprite bank, clearing unused entries only within
the configured capacity. Visible sprites from earlier objects consume capacity
first. Every nonempty changed component independently queues one tile job,
even if hidden, clipped or beyond sprite capacity. A full 64-job queue drops
new jobs individually. Component tile positions advance independently of
visibility. Forced refresh applies to every listed object before being cleared;
with no objects, it stays set.

Each job captures its own borrowed shape/tile pointers and lengths, shape
offset, count and destination tile. Rebinding an object's assets or removing
its record before head therefore cannot change the queued resource identity.
The underlying bytes must remain alive and unchanged through upload. Head
validates the entire queue before writes, uploads tiles in FIFO order, then
uploads the previously prepared bank and updates the original flags. Tail,
head, reset and registration do not advance the scheduler or input. The caller
retains responsibility for original head/tail enable and traversal order.

All bounded tail/head validation completes before mutation. Malformed queues,
resources, frame selections and list state return -1 without partially
changing records, buffers or hardware output.

## Verification

The cartridge-free C suite covers:

- all 65,536 single-component flag/delay pairs and independent object clocks;
- all 65,536 horizontal and 65,536 vertical clipping coordinates;
- 32-object registration/saturation, stable priorities and every removal slot;
- two-component frames, terminal empty shapes and deferred resource identity;
- all queue occupancies 0 through 64, including a partially admitted badge;
- malformed lists, resources, animation headers, jobs and atomic rejection;
- reset preservation and removal before upload.

The private original oracle compares all modeled nonstack RAM, all 32 object
records and list entries, frame/delay/bounds, both sprite/palette banks, tile
job bytes, 8192 tile bytes, 256 hardware sprite bytes and 64 palette indices.
The checked run covers 282 cases and 5,468 updates. It includes the eight tee
animation choices, priority changes, flagged and
explicit removal, reset, queue saturation, repeated tails, early/repeated
heads and exact clipping edges. It uses the existing isolated original core
without rebuilding it. Generated reports and all private resources remain in
ignored `build/tee-object-check/` paths.

Host, ASan/UBSan and QEMU m68020 run the same cartridge-free C suite. The module
also compiles freestanding for m68020; its only external symbol is the existing
`ntm_display_reset`. The legacy cursor API remains unchanged. Its clipping equality check was corrected using the
same original evidence, with a separate 1,176-case original edge oracle across
selector, title and round prompt resources. The synthetic legacy cursor test
now exhausts both horizontal and vertical word coordinates. These are bounded
equivalence tests, not a complete game conversion or target performance
measurement.

```
python3 -m unittest discover -s tests -p 'test_tee_objects.py' -v
bash tools/check-tee-objects.sh
```

The full script requires the private cartridge, the existing screen-probe core,
and m68k Linux GCC/QEMU on PATH. `CC`, `CROSS_CC` and `QEMU` can override the
compiler/emulator commands. It does not install tools or rebuild the original
core. Its final JSON is the authoritative case/update count for that run.

The targeted legacy regression command is `bash tools/check-cursor-edges.sh`.
It runs all three single-object C suites on host, ASan/UBSan and QEMU m68020,
then the existing cursor/queue/title/prompt original oracles and the new edge
oracle. The `NTM_CURSOR_LIBRARY` override keeps its freshly compiled library
separate from other workers' native libraries. Neither check script rebuilds
the original probe core.

## First-tee banner extension

The bounded service also accepts tag 24, graphics index 56, at precisely four
animation offsets: 60 for registration, 152 for arrival, 148 for the first-tee
portrait, and 156 for exit. Other offsets remain rejected. The borrowed animation
array now has 40 slots so the original offset divided by four can address slot
39 without remapping gameplay state. Unused slots do not imply supported art.

A separate original oracle passes 80 flows and 6,400 tail/head updates, comparing
all modeled non-stack RAM, object records, ordering, bounds, queued uploads, both
sprite banks/palettes and graphics bytes. It exercises the four original
animations, clipped/offscreen positions, flips, freezing, restart and removal.
The prior 282-case/5,468-update overview oracle also passes. Synthetic registration
tests check every offset below 160 and reject all but the four supported choices;
missing borrowed animation data fails atomically. Host, sanitizer and m68020
regressions pass. Run `bash tools/check-shot-objects.sh` for the private original
comparison. The resource resolver is authored code, not embedded game art.

This service does not by itself implement the banner's task lifecycle or its
per-scanline plane-1 scrolling. Existing hardware evidence at checkpoint 299f589
uses its original eight-slot asset layout and is not a new-build measurement.

## First-shot scene object

Normal first-shot setup accepts tag 5, indices 68/72/76 and animation offsets
0,4,...68 for the three courses and eighteen holes. This is the original slot-4
object registered by 209e3a. Registration initially uses descriptor index 68 and
velocity 0, then the producer installs the selected course and hole. Its tile
base is 156, which is not its animation velocity. No asset-layout or generic
parser change is required. Other tag-5 index/velocity combinations are rejected.

`bash tools/check-aim-objects.sh` checks the actual resources for all 54 course/
hole combinations across 1,080 cases / 21,600 tail/head rounds, including flips,
freeze/restart, clipping and removal. It compares all
nonstack RAM, object/list/bounds state, queues, both sprite banks and palettes,
all tiles and hardware sprite data. Public tests check all 160 candidate
velocity offsets for each supported index and missing-resource rejection.
Existing tee/banner services remain separate regression gates. This service
evidence does not establish a playable shot, scene initialization or target speed.
