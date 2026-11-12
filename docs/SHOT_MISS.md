# Native missed-shot notice child

`shot_miss.c` converts original entry `2109d5`, timer callback `210a27`, and
object helper `210a3b`. State, resources, task, and the separate `413b` byte
are caller-owned. No original resource bytes are embedded.

Entry sets `413b` to 255, copies terrain `4270` into `4271`, performs the
original wrapped unsigned gate against slot 5's screen-relative X coordinate,
and queues the real notice at map offset `0x1ca` with tile `0x22f8`. The gate
changes slot 5 byte 15 from 3 to 2 only within the inclusive interval.

The delay starts at 30 and decrements in the entry call. After 31 dispatches
including entry, it installs continuation `210a38` and reaches the exact
boundary before shared post-shot routine `210b74`. This is not task completion:
`210b74` may allocate child `2114e8`, wait for its result and perform scoring
and other post-shot helpers. This slice leaves the owner's result and original
completion byte `41e6` unchanged. No fabricated child, flag, physics or scoring.
There are no sound API calls in this bounded original slice.

Verification uses an unchanged original core and compares complete nonstack
work RAM, video/hardware, notice queue identity, and exact boundary position.
A separate drawing oracle drains the actual notice with original `20054f` and
compares complete work RAM, maps, tiles and hardware. Reproduce with
`tools/check-shot-miss.sh`. Authored tests cover all 65,536 word values for
wrapped coordinate gating and signed countdown, the genuine 31-call wait and
untouched owner result; host, sanitizer, and m68020/QEMU builds are supported.
These are functional tests, not stock A1200 timing or playability claims.

## Follow-on perfect-shot investigation

Original `20c64e` is a separate timed display child, not a stub. It resets the
viewport and scroll bytes, requests palette commands `0x48` and `0x50`, resolves
player-dependent art through `20db4b` (table `21ee99`), and queues group `0x4c`
art at tile `0x40` plus group `0x88` art at tile `0x132`. It waits with delay 2,
opens the viewport in `20c707`, waits with the task-local duration 30, then
really finishes through `20c740` / `20132b`. Implementing this requires the
actual palettes, both drawing resources, hardware bytes and genuine task
lifecycle. That investigation is not an implementation or completion claim.

## Verified slice results

Both default and private-copy backends passed 3,072 original-state cases,
including 439 exact post-shot boundaries, plus 16 original notice drawing
cases per backend. Authored 65,536-word gate/countdown tests passed on host,
ASan/UBSan and m68020/QEMU. No original core, prior source, or target was changed.
