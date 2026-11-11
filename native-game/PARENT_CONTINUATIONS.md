# Native parent timers and randomized selection

Later integration: [COURSE_SCREEN.md](COURSE_SCREEN.md) adds native deferred
course-label drawing and the parent render adapter. The older measurements and
stubbed reference-flow tests described below retain their original scope.

This checkpoint extends the native parent state machine through its timers and
complete modified-confirm selection sequence. It remains an independent
reimplementation with explicit rendering/audio/setup service boundaries, not a
playable port or recovered old runtime.

## Converted behavior

Four original timer callbacks decrement a 16-bit value modulo 65536 and continue
when the resulting sign bit is set. They do not simply test zero. Thus 0 becomes
65535 and continues, whereas 32768 becomes 32767 and returns. Native code uses
unsigned arithmetic and a sign-bit test without implementation-defined casts.

- Cancellation delay expires: clear parent.substate and delay, change to the fade
  callback, then return. The next eligible fade-timer invocation requests the
  unconverted viewport-animation helper and suspends before it executes.
- Acceptance delay expires: map selection 0,1,2 to 2,0,1, update mapped-course state,
  change to the setup callback, then request the unconverted course-setup service.
  The native bounded contract requires a selection in 0..2; the original unchecked
  table access outside that range is not reproduced.
- Random-selection delay expires: change to the cycle callback and decrement the
  remaining byte modulo 256. Nonzero requests another random draw immediately.
  Zero enters the already converted ordinary sound 0/render/acceptance-delay path.
- A completed random draw sets selection to the unsigned byte modulo 3, requests
  render, then sound 2. After those services finish, set random-delay callback and
  effective delay 1 (original stores 2 and immediately decrements).

The initial modified-confirm path sets remaining count 30. A complete sequence
performs 30 draws, 30 sound 2 requests, then sound 0/render and the acceptance delay.
It reaches the next course-setup service boundary after **91 parent callback
invocations**, with 31 render and 31 sound requests. These are callback invocations,
not an asserted number of physical display frames.

Pending services cannot be bypassed by calling the tick function again. Resume
is valid only after the matching sound/render request completed. Fade and course
setup currently remain pending, with resume rejected unchanged. Their backends,
subsequent exit transitions and actual game-loop adapter remain unfinished.

## Native random-byte provider

The original 0x20197b is a deterministic lookup, not an arithmetic PRNG. It reads
shared cursor byte 0x400e, indexes the cartridge table using signed8(cursor),
returns that byte, and increments the shared cursor modulo 256. The native API
accepts a private 256-byte window centered 128 bytes before the original pointer:

    value = window[cursor ^ 128]
    cursor = (cursor + 1) modulo 256

`ntm_course_parent_generate_random` completes a pending draw with this native
provider and then performs the native selection transition. It checks the pending
phase before consuming the cursor, preventing accidental duplicate consumption.
The lower-level supplied-byte API remains useful for service adapters and tests.

No table bytes are published or synthesized as a replacement for the original.
The caller supplies the authorized asset window. The reference obtains it from
the original pointer at 0x260010. The cursor is shared with other game callers and
must not be reset or privately forked at animation entry. Original boot seeds it
from the sum of six RTC bytes; startup seeding and whole-game call ordering are
separate integration work. This is state/data equivalence, not CPU-flag/cycle ABI.

## Original and native validation

**262,912 new original/native differential cases pass:**

- Every 65536 timer bit pattern for each of four callbacks: 262144 cases.
- Every 256 provider cursor, with preserved upper accumulator/W state checked.
- Complete 91-invocation modified selection for every starting shared cursor:
  256 full flows. The original provider actually executes, rather than receiving
  a mocked random answer. Sound/render calls remain controlled service stubs.
- Every 256 wrapping remaining-count value at random-timer expiry.

Each reached sound/render/random/fade/setup boundary is checked. All non-stack RAM
is compared. The same 16-byte original stack-scratch allowance from the parent
harness is retained. Original RAM/registers are restored afterward; running all
six differential suites before navigation leaves its complete trace identical to
running without them.

Independent native tests cover all 262144 timer patterns, byte boundaries, a full
91-callback flow, 65536 synthetic table/cursor draws and duplicate-supply rejection.
Host, ASan/UBSan, m68020 execution and the freestanding Amiga build pass.

Five original live traces retain exact video/audio/final state and repeat exactly
over 12,000 display frames. They contain **50 verified provider calls**: 30 in
modified confirmation, 10 fairway and 10 O.B. Navigation and cancel contain zero
provider calls, so their unchanged traces are not provider coverage. Prior input,
selector, lifecycle and allocation checks remain active. Parent timer transitions
are checked in isolated differential flows, not yet replaced in the live game.

## Stock target fixture

The new parent-flow fixture runs the native initialization, cleanup, timers,
provider and staged parent transitions on the emulated stock PAL A1200: 68020,
2 MiB Chip, zero Fast, full eight-plane display DMA. CIA calibration is 20.0316 ms.
It supplies an authored synthetic table and counts successful sound/render stubs.
It stops before fade/course setup. 128 flows per variant run forward and reverse.

| Flow | Cumulative logic time per flow | Average per parent callback |
| --- | ---: | ---: |
| Ordinary accept, 31 callbacks | 4.294–4.333ms | 138.5–139.8us |
| Cancel to fade request, 7 callbacks | 1.567ms | 223.8us |
| Modified accept, 91 callbacks | 25.220–25.260ms | 277.1–277.6us |

The 25 ms figure is the sum across 91 callbacks, **not a rendered-frame time or FPS**.
There is no real-time pacing in this fixture. Timing includes synthetic node setup
and invariant checks, and excludes actual sound/render/fade/setup work. These are
hot repeated-workload averages, not worst-case callback bounds, physical-hardware
measurements or an original-machine speed ratio. Final task graph, state, request
counts and cursor are verified independently. A fresh target run matches the
entire timing report byte for byte.

Use `make -C native-game continuations continuations-sanitize continuations-m68020`,
`bash tools/build-reference.sh --parent`, and `reference/run.py` with
`--continuation-cases --random-check` and the private inputs. Compare live traces
with `reference/compare_random_trace.py`. Target build/run use
`--parent-flow-benchmark`. Public CI compiles the reference and runs synthetic
checks, without private cartridge/firmware inputs.

## Service and exit follow-on

[GAME_SERVICES.md](GAME_SERVICES.md) converts fade, valid course setup, the bounded
queued-object initializer and exit reset. Parent flows can now reach owner-result
delivery and task removal while sound/course-screen drawing remain explicit
stubs in the differential harness. The earlier timing and boundary results above
remain scoped to their original checkpoint.
