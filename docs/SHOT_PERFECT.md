# Native perfect-shot notice child

This additive module converts entry `20c64e`, delays `20c6f6` and `20c72f`,
viewport opening `20c707`, and real finish `20c740`/`20132b`. It uses the exact
player-art selection from helper `20db4b` via a private original resource
resolver. No original bytes are embedded in published source.

Entry stores task-local kind 0 and duration 30, sets viewport `[0,0,0,152]`,
clears both layers' hardware scroll bytes, requests palette commands `0x48`
and `0x50`, and queues the player art and overlay in their original order.
The first delay starts at 2 and decrements during entry. Expiration opens the
viewport to `[0,0,160,152]`, copies the local duration to the task delay and
executes the next decrement in the same dispatch. When that duration expires,
the genuine task finish routine defers, unlinks or recycles according to real
priority and flags. Ordinary complete flow takes 33 calls including entry.
No owner result or parent completion flag is synthesized.

State and resources are caller-owned. The resource contract contains two
palette jobs, six player artworks and one overlay. The context uses existing
intro viewport, scroll, screen and palette state plus the actual task context.
This slice makes no sound requests. It is a notice display, not ball physics.

`tools/check-shot-perfect.sh` reproduces host, sanitizer, optional m68020/QEMU,
original-state and original queued-drawing comparisons. The full-state oracle
covers all six golfers, palette/stamp saturation, signed and wrapped timers,
viewport bytes, and the genuine deferred/unlink/recycle task paths. It compares
all nonstack RAM and complete hardware/video; no original helpers are replaced.
The drawing oracle drains both actual artworks through original `20054f` and
compares all maps, tiles, hardware and nonstack RAM. Palette commands/descriptors
are checked at queue production; palette head/tail are existing shared services.

The preserved Amiga target is unchanged. These are functional correctness
checks, not A1200 speed, memory-capacity or full-game playability measurements.

Verified: both default and private-copy backends passed 2,304 original-state
cases and 24 original paired-art drawing cases per backend. Authored 1,024
complete 33-dispatch notice flows passed host, ASan/UBSan and m68020/QEMU.
