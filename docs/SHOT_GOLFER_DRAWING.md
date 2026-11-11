# Real golfer initial drawing callback

`shot_golfer_drawing.c` converts the complete original `20a5aa` callback through
its return at `20a5c8`. It fills the earlier golfer initializer's real drawing
boundary; it does not jump over it.

The converted path:

1. Draws both `20ab15` scenery rims immediately, including the reflected rim.
2. Runs the actual `21081d` feature collection and selection on the current
   hole's real feature list.
3. Applies the original terrain eligibility and all five unsigned depth choices.
   The fifth choice clears the real temporary map region and returns without
   inventing green art, new geometry, or an object.
4. Draws the selected green art immediately, computes signed horizontal
   projection `20fa2f`, resolves the bounded clipping record, and performs the
   original nine forward word-copy rows. Source/destination overlap keeps the
   real `LDIRW` behavior.
5. Registers the actual tag 52 object in slot 18 only for the first two drawing
   choices, with genuine animation selectors 20 and 16. Full object lists retain
   the original dropped-registration behavior and synchronized tag views.
6. Runs the real pin adjustment `20a94a`, saves the real HUD map `20ab9f`, installs
   the actual `20a5c0` adapter, and performs its same-call animation tick.

The immediate drawing requests are distinct from deferred stamp jobs. Existing
queued requests remain pending until the genuine drawing head. No native
callback pointer is called as a stand-in for original code, and no child is
marked complete. The recurring callback remains the independently verified
`ntm_shot_golfer_animation_tick`.

## State and resource contract

The context borrows the existing scene, tile memory, map memory, objects and
display queues. Drawing state owns the previously verified feature-selection
scratch and the original green center/half-width words. Existing caller scratch
and list bytes survive whenever the original leaves them untouched.

All required resources, clipping bounds, object registration and animation
dispatch are validated before live outputs change. This uses the public
read-only stamp validator and does not allocate speculative copies of the full
video arrays. Asset spans must not overlap destination state or video memory.

Resource spans come from private resolvers. No extracted image, cartridge bytes,
original engine instructions or private input hashes are embedded in source.
The current-hole terrain maps needed by the earlier initializer remain a
separate loading requirement.

## Verification

`tools/check-shot-golfer-drawing.sh` checks 1,200 authored whole-callback cases
on host, ASan/UBSan and static m68020/QEMU. It covers empty/eligible/blocked
features, every depth choice, animation dispatch, queue saturation, full object
lists, callback identity, saved map contents and atomic rejection.

Each default/private asset-copy backend passes the unchanged-original oracle:

- 1,200 complete `20a5aa` callbacks, using all 54 actual hole lists plus authored
  boundary geometry. All five depth choices occur.
- 62 real tag 52 registrations and 360 subsequent genuine drawing-head drains.
- 2,088 independent `20fa2f` projection calls, including full-width signed words
  and zero/denominator boundaries.

The oracle compares all nonstack game RAM and hardware/video bytes from `8000`
through `bfff`, including immediate pixels and preserved pending jobs. Only
measured original stack writes are excluded. Tag 52 and reflected stamping also
have their own independent rendering and old-family regression suites.

## Remaining integration boundary

This is native source coverage, not a newly playable or full-speed Amiga build.
The current control application, exporter and preserved target disks remain
unchanged. The next work is to bind these real children into the scheduled fire
scene, load and budget their actual resources, then verify the combined original
scheduler/input/frame path and a separately labeled stock PAL A1200 target.
Power selection, cancel transitions and ball flight remain explicit later work.

## Combined resource audit

`reference/report_shot_fire_resources.py` merges overlapping original source
spans, aligns the pool, packs it and verifies lossless roundtrips. The existing
control pool is 596,238 bytes unpacked / 306,527 packed. Complete static first-fire
resources increase this to 653,200 / 340,807: an increment of 56,962 / 34,280.

All 54 terrain and feature sets sum to 1,293,993 span bytes but overlap heavily:
the merged aligned pool is 141,140 bytes, packed to 74,636. This corrects any
interpretation of the earlier sum as a required resident-memory budget. Separate
per-hole files duplicate shared ranges and are not automatically the best disk
layout. The first-hole standalone sets are 6,621 / 5,469, 6,449 / 5,176 and
5,897 / 4,918 unpacked / packed bytes across the three courses.

These measurements exclude structures, code, stack, assembled palettes, working
buffers, loader and filesystem overhead. The preserved control target has only
69,712 Chip bytes and 7,168 DD bytes free. Adding resources to its existing
embedded layout will not fit unchanged. Loading compressed resources into
temporary storage and releasing that storage afterward is an unimplemented
option, not recovered memory or a stock-target result. No courses or art were
removed, and no new target binary was built for this audit.
