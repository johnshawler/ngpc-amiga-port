# Native selector task lifecycle and bounded dispatch

This is a new native state-level implementation, not recovered former runtime.
It extends the verified input and course selector with actual task completion
and one original dispatch iteration. The original rendering checkpoint is intact.
No cartridge, firmware, original machine-code bytes, captures or private trace
hashes are included.

## Original behavior retained

Original task fields are callback+0, previous+4, next+8, owner+12, unsigned priority
byte+16, result+17 and flags+18. Native nodes use actual C pointers and callbacks;
they are not a binary-compatible original RAM structure.

Completion at 0x20132b compares current priority with next priority unsigned.
When current is lower, only the callback changes to the completion callback, so
removal is deferred. Otherwise previous.next and next.previous are linked around
current. If flags bit0 is clear, current.next becomes the old free head and the
free head becomes current. If bit0 is set, current's own links are preserved.
The returned scheduler cursor is previous in either unlink case. Owner, priority,
result and flags are not reset. Recycling is list insertion, not memory freeing.

The native course callback writes its owner's result, dispatches accepted movement
as sound-request argument2 through an application callback, and invokes completion
on confirm/cancel. A deferred node subsequently runs only completion, so later
buttons cannot overwrite the already-delivered result.

Original dispatch at 0x20137c publishes the entry cursor, calls its callback, then
advances from the callback-returned cursor's next link. This distinction is vital:
removal returns previous, whose new next is the correct successor. The native
`ntm_task_dispatch_one` implements exactly one such iteration, not an inferred
frame scheduler. A published entry pointer may refer to a recycled node; its
storage must remain caller-owned and valid.

Valid task storage, coherent links, current==task at completion entry, and no
overlapping nodes are required. A two-node ring with previous==next is supported.
Malformed links and self-loop recycling are outside the bounded contract. Native
validation is intentionally stricter for malformed previous pointers than the
original deferred path. The original free sentinel is 0xffffffff; adapters map
that identity to a native sentinel/null without dereferencing it.

## Validation

139,264 isolated original/native differential cases pass:

- 131,072 completion cases: every unsigned current/next priority pair and both
  recycle-flag classes, with other flag bits set. Full RAM changes, cursor and
  original PUSH/POP scratch are accounted for.
- 4,096 iterations of the actual original dispatcher around completion, checking
  published cursor, callback-mutated advance, stack and all RAM.
- 4,096 composed course→completion→dispatch cases, checking selection, owner
  result, sound target/argument and all non-stack RAM. Sound requests are observed
  and skipped at the call boundary, preserving original surrounding PUSH/POP.
  Only eight bytes of known stack scratch are exempted in these composed cases.

The original CPU state/RAM is restored before live emulation. Native independent
unit tests cover 131,072 priority/flag dispatch cases, two-node aliasing, and an
input→navigation→confirm→deferred-completion→recycling sequence. Host, ASan/UBSan
and QEMU m68020 pass. Freestanding m68020 and bootable target build pass.

Five original-game traces, 12,000 display frames total, preserve video/audio/final
serialized state exactly and match fresh instrumented repeats. They contain 6,000
input updates, 220 course-handler calls, and 137 completion calls: navigation15,
cancel15, Option+A15, fairway44, O.B.48. All 137 live calls unlink and recycle;
**live coverage does not demonstrate deferred or nonrecycled removal**. Those
branches are covered by the isolated differential and native tests.

## Stock target timing

The emulator-only fixture uses stock PAL A1200, 68020, AGA, 2 MiB Chip, zero Fast,
cycle-exact CPU/memory and full eight-plane 320x256 display DMA. CIA timing is
calibrated against a PAL frame (20.0217ms). Each of four variants runs 16,384 calls,
then repeats in reverse order. Same input stream and course outcomes are checked.

| Workload | Forward | Reverse |
| --- | ---: | ---: |
| Input + course, B enabled | 244.649 us/call | 244.643 us/call |
| Add lifecycle/dispatch, B enabled | 484.818 us/call | 484.611 us/call |
| Input + course, B disabled | 243.637 us/call | 243.776 us/call |
| Add lifecycle/dispatch, B disabled | 483.635 us/call | 483.537 us/call |

The integrated rows include synthetic node rearming and invariant checks every
call. They are **not a pure dispatch-overhead measurement**, allocation timing,
cache-cold bound, original-machine speed ratio or whole-game FPS. The same binary
baseline is the comparison; do not compare layout-sensitive numbers across older
binaries. The stream produces65 movement requests and24/16 finishes;16 finishes
are deferred in each integrated variant. Sound is counted, not played. Target
final control/selection/result data and counts match an independent Python model.
A fresh target run matches the timing report byte for byte. All66 local tests pass;
ASan/UBSan and m68020 execution also pass.

## Remaining work

This is not a playable interactive course screen. The caller still needs native
allocation, course-parent initialization and wait-state transitions, descendant
cleanup, the rendering child, input mapping/pacing, and real sound services. The
original dispatcher is an infinite callback loop with no inherent frame boundary;
we do not fabricate one. Other original callbacks remain unconverted. Course shot
physics and full-game rendering throughput are still outstanding.

Reproduce using `make -C native-game lifecycle lifecycle-sanitize lifecycle-m68020`,
`bash tools/build-reference.sh --lifecycle`, and `reference/run.py` flags
`--lifecycle-cases --lifecycle-check` with private authorized inputs. Use the five
existing course/gameplay button scripts and `reference/compare_lifecycle_trace.py`.
Target build/run use `--lifecycle-benchmark` and the same configured toolchain,
PUAE and private Kickstart as the earlier fixtures. Public CI builds the observer
and runs synthetic/no-cartridge tests; it does not run the private gameplay traces
or stock-target firmware fixture.

Allocation, descendant cleanup and initial course-parent transitions are now
available in [COURSE_PARENT.md](COURSE_PARENT.md), with explicitly suspended
external service requests. The earlier timing above belongs to this document's
original lifecycle checkpoint, not to the later parent/allocation code.
