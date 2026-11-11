# Native aiming club control

`ntm_shot_club_control` converts the complete original `20ad1a` child tick,
including its `20ad2b` input branch, `20adb0` timer, `20adcb` redraw and
`20ae10` highlight. It does not implement a swing or complete the child.

Pass the active player's current and rising-edge bytes from the existing
input state. Do not sample host buttons or generate new edges in this child.

- Original signed `reset_d1 >= 2` returns before all control and timer work.
  Byte values 128–255 are negative and take the other path.
- Changes are inhibited by `reset_4416`, hole byte `0x77`, or either held
  left/right direction. Timer/restoration work still runs while inhibited.
- Exactly one rising up/down edge changes the club. Both edges together do
  nothing. High input bits do not change this choice. Original signed-byte
  bounds wrap the club between its initialized minimum and maximum.
- A change sets marker dirty byte `change_flag` to 255, requests sound, and
  starts a 32-update highlight timer. The same tick decrements it to 31.
  Repeated presses restart the timer; a held button alone does not repeat.
- Redrawing preserves the original label, three digits, lie, power limit
  and power-gauge position. While the timer remains nonzero it adds the
  original club highlight at map offset `0x10e`, palette/tile `0x20e4` for
  an unmodified player or `0x3ae4` for a modified player.
- Expiration restores the exact four saved eight-byte HUD rows. It does not
  redraw or clear the marker dirty byte. Queued jobs remain deferred.
- Existing final-digit queue overflow behavior still selects the separately
  resolved lie span at the original base plus `0x300`. A saturated queue may
  drop the highlight, but the timer and later map restoration still occur.

The resource struct appends 23 resolved highlight assets. All other resource
spans, the club state layout, initialization and callback identity are unchanged.
The existing bounded `ntm_shot_club_tick` remains available for old first-open
callers; interactive callers must use `ntm_shot_club_control`.

The existing shared sound request receives `(service=2, command=0,
parameter=0)`, corresponding to original `25001c` with WA command and C
parameter. This is a request boundary, not native audio synthesis, and is
separate from selector sound service `250018`.

## Verification

The authored `shot-club`, `shot-club-sanitize` and `shot-club-m68020` targets
include 630 initialization and 2,560 control cases. They check input masks,
signed state gates, every byte timer, pending deltas, both wraps, repeated
presses, modifier highlights, drawing, map restoration and sound requests.

`tools/check-shot-club-control.sh` runs unchanged-original private oracles for
both ordinary and private asset-copy backends. Each passes 540 initialization
cases, 6,442 control comparisons and 4,440 drawing-head drains. The tests
compare every nonstack
RAM byte, callback identity, maps, tiles, exact queued resource identities and
sound requests. They cover twelve power tables, fifteen terrains, seven queue
thresholds, every legal min/max range, all held bytes, all byte state/timer
values, blocked control and multi-tick highlight expiration. Only the explicit
audio request service is intercepted; control, drawing and timer helpers run
unchanged. Original resources are resolved privately and are never embedded in
source. No full-game speed or target memory-fit claim follows from these tests.

The target exporter resolves all 23 highlights. Independent exported-binding
verification passes 3,816 exact span checks and preserves all 96 packed intro
arrays, including their C sizes. Those checks use a separate ignored output
directory and do not overwrite the preserved first-open target.
