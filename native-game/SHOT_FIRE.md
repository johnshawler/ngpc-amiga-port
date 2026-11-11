# First fire transition and waiting marker

`shot_fire.c` converts the real `209919` parent entry, both following signed
countdowns, and the next input gate. It is an isolated slice. The current
interactive aiming application deliberately still stops before the first fire
transition; this module does not make that application a playable shot.

The entry sets the information-overlay byte to 255 and shot lifecycle to 1,
closes the viewport, disables incremental world scrolling, and removes actual
object slots 0,2,5,17. It clears the original four-by-four HUD region, queues the
privately resolved color-mode palette command 0x10, and allocates the real spin,
trajectory and golfer child callbacks in original order. Free-list exhaustion
preserves the actual fallback behavior and stale child result bytes. The parent
installs `2099a5` with delay 3 after the genuine same-call decrement.

No child callback is invoked or replaced with a success stub. The application
must not dispatch any unconverted child or call parent waits to bypass it.
`shot_spin` and `shot_trajectory` provide separately verified bounded children;
`shot_golfer` now provides its real initializer, while its later callbacks remain
separate work.

The first wait opens the viewport and clears the overlay byte, then installs
`2099e1` with its own same-call decrement 4 to 3. Both waits return while the
wrapped decremented word is signed nonnegative, including zero. The subsequent
`2099f2` input gate gives cancel bit 5 priority over fire bit 4. It stops before
`209b71` or `209a0a`, preserving those transitions as explicit boundaries without
invented power, cancellation or ball-flight effects.

`ntm_shot_fire_markers_tick` supplies the real same-call `20af8a` to `20afa5`
transition and `20b1d6` waiting refresh. Dirty waiting markers update direction
fields and the miniature marker only. They do not change the aiming angle or
move the removed main marker. Lifecycle 2 performs genuine task completion,
including priority deferral and free/active links. Other lifecycle byte values
retain the original dirty-gated behavior. The earlier bounded marker API remains
unchanged.

## Verification

`tools/check-shot-fire.sh` runs authored C tests on the host, with ASan/UBSan,
and on m68020/QEMU, then compares the unchanged original through
`reference/screen_probe.py`. The original core is not patched or recompiled.

- 576 parent entries cover object-list permutations, zero to three free tasks,
  all palette queue limits, real removals and deferred palette identities.
- 13,056 parent ticks cover every pressed byte and signed delay boundaries,
  stopping before actual power/cancel actions.
- 2,048 marker cases cover same-call transitions, all lifecycle bytes, dirty
  refresh, wrapped angles/lengths, priority deferral and task links.

Comparisons check all nonstack work RAM and hardware/video bytes, excluding only
measured original stack writes. The public fixture has 576 entries, 786,432
wait/input cases and 2,048 marker cases. Original resource bytes, extracted art,
captures and private input fingerprints are not embedded in source.

`shot_projection.c` separately converts original `20f98a`, including exact
signed `20d541` division, wrapped fixed-point arithmetic and both scratch words.
`tools/check-shot-projection.sh` checks 2,342 unchanged-original full-width and
boundary cases plus 8,192 rational/cardinal and 8,192 wrapped-translation cases
on host, sanitizers and m68020/QEMU. No floating-point or 64-bit runtime arithmetic
is used. This helper is preparation for the real golfer scene, not a renderer
or completed swing.
