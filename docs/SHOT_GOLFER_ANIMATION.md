# Bounded golfer scenery animation

`shot_golfer_animation.c` converts the complete original `20a5c0` callback:
ground dispatcher `20ac01`, foreground dispatcher `20ac50`, and their shared
timer/frame helper `20ac7e`. It also converts the complete pin adjustment
helper `20a94a`.

The real earlier drawing setup `20a5aa` is now implemented in
`shot_golfer_drawing.c`. The application must run it before this animation callback.
These routines are verified building blocks, not an interactive swing or a new
Amiga game build.

## Behavior and ownership

- The four ground animation kinds and single foreground animation use privately
  resolved original four-frame sequences. Static kind 252 returns without
  touching frame, timer, reload, or kind bytes.
- Timer zero wraps to 255. Timer one reloads, advances the frame by two modulo
  eight, and enqueues the actual scenery stamp. State advances even when the
  queue is full. The ground request precedes the foreground request.
- Unrecognized dispatch kinds and odd expiring frame indices stop explicitly
  before mutation. The original initializer produces supported kinds and even
  frame offsets. Odd offsets are preserved while their timer runs.
- Pin adjustment preserves original signed distance and animation comparisons,
  wrapped green bounds, screen-edge checks, attribute-byte changes, and terrain
  6/7 animation selection. It does not invent green geometry. The caller must
  supply the actual earlier drawing stage's center, half-width, and status.
- No callback is installed, child completed, input sampled, or drawing head
  consumed. Both animation channels are validated before either changes.

Only the eight existing task scratch bytes are needed for animation state.
Resources are borrowed. The 20 resolved frame slots refer to 17 distinct stamps;
15 are new beyond the existing golfer initializer's resources and contain 3,019
descriptor bytes. Their tile prefix fits inside that initializer's existing
14,464-byte scenery tile prefix. This excludes structures and code, and does not
assert that the stock target or DD disk has enough room. The target exporter and
preserved control frontend are unchanged.

## Verification

Run `tools/check-shot-golfer-animation.sh` with the local m68k toolchain active.
The authored fixture runs 4,608 callback cases plus 262,288 pin arithmetic cases
on host, ASan/UBSan, and static m68020/QEMU. These include every 8-bit countdown,
every real animation frame, simultaneous expiration, queue saturation and order,
every signed 16-bit pin distance and animation value, and atomic rejection.

Each ordinary/private asset-copy backend is compared with the unchanged original:

- 4,608 complete animation callbacks.
- 4,096 pin adjustments with boundary and randomized full-width coordinates.
- 576 actual drawing-head drains, including simultaneous ground/foreground art.

Comparisons cover all nonstack game RAM and all hardware/video bytes from
`8000` through `bfff`. Only measured original stack writes are excluded. No
original instructions, resources, or engine binaries are patched. Private ROM
data and trace captures remain outside source control.

## Initial drawing dependency

`20a5aa` calls `20a9ce`, then `20a94a`, then the already converted map-saving
helper `20ab9f`; it installs `20a5c0` and performs its first animation tick in the
same call. The separately converted `20a9ce` path includes:

1. Two real scenery stamps from `20ab15`.
2. `21081d`: nearby-feature collection `2100ca`, direction transform and view
   filtering `21082a`, then nearest-feature selection `21098c`.
3. Actual surface eligibility, distance-dependent green art, signed projected
   horizontal position `20fa2f`, and clipped map-row copies `20ab59/20ab77`.
4. Optional real tag 52 object registration and its asset/rendering support.

The collection helper reads the current hole's feature list at its resolved
offset `36`, selects up to 16 entries in the original 200-unit rectangle, and
retains original scratch and list terminators. The drawing module consumes this verified helper and real hole resources;
application integration must bind them before using the green state.
