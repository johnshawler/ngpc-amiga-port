# Shot spin child

`native-game/shot_spin.c` converts original initializer `20b604`, bounds resolver
`20db7c`, control `20b62d`, directional adjustment `20b64a`, timer/redraw `20b6b0`,
indicator producer `20a556`, map restoration `20abd0`, and the existing `20132b`
task finish path. It does not implement the actual swing or ball flight.

## Resources and adapters

`ntm_shot_spin_resources.bounds` borrows twelve two-byte records. The private
resolver reads the original table from `21ee9f`; no original bytes, art, captures
or private hashes are checked in. The initializer uses byte-wrapped golfer plus
modifier and doubled indexing, loads task minimum at `6016` and maximum at
`6017`, caches hole spin `4276`, clears timer `4277`, and installs the supplied
wait callback. The initial bounded version's field names were reversed; the
byte layout is unchanged and the names now express their actual meaning.

`ntm_shot_spin_draw_resources` borrows nine indicators and nine highlights:

- Group `009c`, offset `4*(spin+1)`, queued at map offset `0448`, tile `00d6`.
- Group `00ac`, offset `4*spin`, queued at map offset `010e`, tile `28e4`.

The application calls `ntm_shot_spin_control` with the shared scene context,
these drawing resources, lifecycle `41d1`, input gate `41e1`, the active player's
pressed byte, and task context/node for completion. Lifecycle 2 returns without
processing input or timers. Lifecycle 3 calls the existing verified task finish
routine, preserving deferred, unlink-only and recycle outcomes.

Other lifecycle bytes process up/down edges unless the gate is nonzero. Both
edges together are a no-op. Byte-wrapped candidate values are checked using the
original signed bounds comparisons. An accepted change sets timer 32 and issues
shared sound service 2, command 1, parameter 0. The timer then decrements in the
same call. Expiry restores the four saved eight-byte map rows. A changed cached
spin queues the genuine indicator and highlight in order. Queue-full behavior
uses the existing silent-drop producer; restoration and deferred draws remain
separate operations.

The previous `ntm_shot_spin_tick` remains a bounded compatibility API. Interactive
callers should use `control`. Corrupt spin values outside the nine resolved
assets return `NTM_TASK_BOUNDARY` before mutation when redraw would be required.
Invalid/missing caller resources return an error. No fake effects replace an
unavailable resource or callback.

## Verification

After sourcing the local m68k toolchain activation, run:

    sh tools/check-shot-spin.sh

The script runs native C tests, ASan/UBSan, freestanding m68020 compilation, and
a static m68020 test under QEMU. C control coverage includes 6,912 combinations
of spin, input byte and timer, plus lifecycle/boundary/initializer tests.

The original-core suites use unchanged `reference/screen_probe.py`:

- 1,918 initializer/idle/finish cases, including all twelve real bound records,
  both player banks, every noncompletion lifecycle byte, and all flag bytes
  across deferred/unlinked/recycled completion. No intercepted calls.
- 2,185 adjustment/timer/redraw cases, including every timer and input byte,
  every noncompletion lifecycle byte, all legal min/max ranges, queue saturation,
  cached-spin mismatch and expiry. Only the external audio request service is
  intercepted; its exact service/command/parameter is compared.

The control oracle also passes all 2,185 cases with the private-assets queue
implementation enabled.

Both suites compare all 16 KiB of nonstack work RAM, excluding only measured
stack writes, with exact callback and linked-task identity mapping. The control
suite additionally compares existing/new deferred asset identities, immediate
map restoration, unchanged tiles before draining, and exact maps/tiles after
original versus native queue draining. Completion checks the current-task
register. This is spin control verification, not proof of an interactive swing.
