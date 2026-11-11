# Original power parent and cancellation transitions

`shot_power.c` is an independent native translation after the verified first-fire
scene. It does not change the preserved fire application or Amiga target.

## Parent callbacks

- `209a0a` sets lifecycle `41d1` to 2, restores the four saved eight-byte HUD
  rows through the genuine `20abd0` behavior, and allocates `20b70b`. It installs
  `209a2e` with delay 3 after the same-call decrement from 4.
- `209a2e` uses the original wrapped signed delay. Its expiry enters `209a3f`
  in the same call. Cancel bit 5 takes precedence over power bit 4.
- A further power edge sets lifecycle to 3, allocates the real `20bc19` child,
  and installs `209a77` with delay 3. Its expiry enters `209a88` in the same
  call. Club 13 proceeds without another edge; other clubs require bit 4.
  This isolated parent stops **before** `209aa2` swing effects.

Allocations retain the genuine fallback, priority wrapping, active/free links
and stale result/scratch fields. No child callback is called, skipped or
reported complete by the parent module. Applications must bind real children
or stop before their dispatch. `shot_power_meter.c` and `shot_accuracy.c` supply the genuine power and accuracy
children. Their component oracles cover the child behavior independently.

## Two different cancels

First-fire cancellation at `209b71` sets the overlay to 255, closes the viewport,
and removes actual consecutive deeper-priority descendants through the existing
native lifecycle helper. It schedules `209b9a` for the next dispatch. That
callback clears the real 14-by-15 word rectangle at map offset `0x84`, resets
lifecycle to zero and both screen words, and calculates the original camera
origin. It stops before the `2097f0` world reload. It does not claim to have
rebuilt or reopened aiming.

Power-stage cancellation at `209bb0` leaves the viewport and descendants alone.
It schedules `209bc1`, which repeats the original saved four-byte meter
background over twelve rows and sets lifecycle to 1. The following `209bd6`
dispatch returns to the genuine first-fire closed wait with delay 3. This path
never reallocates the first-fire spin, trajectory or scenery children.

The standalone parent exposes terminal phases and refuses another tick after
an explicit boundary or a handoff to the first-fire wait. The application
adapter may resume the real wait after the successful cancellation handoff.

## Verification

`tools/check-shot-power.sh` runs 1,049,633 authored cases on host, ASan/UBSan,
and static m68020/QEMU. They exhaust all 65,536 delay words, consequential
input combinations, allocation exhaustion, byte-priority wrap, descendants
with recyclable and reserved flags, both cancellations, and invalid/boundary
contracts. Test callback identities are not executable child implementations.

The unchanged-original oracle compares 16,864 cases, every pressed byte and
signed delay boundaries. It checks the complete nonstack work RAM and
hardware/video image, task headers, links, callback identities and current
cursor. Only measured original stack writes are excluded. It stops the
original at the same explicit world/swing boundaries. No original instructions
or resources are replaced, and no extracted original bytes are in source.

These are native game-code conversions, not an Amiga performance result or a
completed swing. Ball flight and full-speed gameplay remain unfinished.
