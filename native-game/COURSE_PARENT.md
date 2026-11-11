# Native allocation and initial course-parent transitions

This extends the independent native reimplementation. It does not recover the
former lost runtime or make the game playable. No original assets are included.

## Allocation and cleanup

`ntm_task_allocate` translates original 0x2012e7 with a caller-owned free list.
It removes the free head, sets its callback/owner/wrapping unsigned priority+1,
inserts it immediately after the parent, and clears flags. It deliberately
preserves the node's stale result and other untouched data. No OS allocation
occurs. Native NULL represents the original 0xffffffff free-list terminator.
When exhausted, it returns the supplied reserved fallback without initializing
or linking it, matching the original reserved object 0x4ee6 behavior.

The active/free lists must be disjoint, coherent and caller-owned. The caller
supplies a valid reserved fallback. Malformed-pointer behavior is not emulated.

`ntm_task_remove_descendants` translates original 0x201363: while the next node's
unsigned priority exceeds the parent's, unlink it unconditionally and recycle
when flags bit 0 is clear. Preserve the parent cursor. This intentionally differs
from normal completion, which may defer. Unsigned priority wrapping is retained:
a parent with priority 255 creates children with priority 0, which this cleanup
does not consider deeper descendants. Do not replace that behavior with a
supposedly safer depth calculation.

## Parent initialization

`ntm_course_parent_initialize` translates original 0x209395. Allocate the renderer
first, the selector second, then clear parent.result and set its wait callback.
Insertion-after-parent produces parent→selector→renderer order. Child result
bytes remain stale. Initialization still changes the parent when the pool has
zero or only one free node, matching the original ignored allocation return.

Native renderer/wait/continuation callback identities are supplied by the caller.
The selector callback is the previously verified native `ntm_course_task`.
This module does not implement the rendering callback or bind a full game loop.

## Initial wait and explicit service continuations

`ntm_course_parent_begin` translates the initial wait at 0x2093b8:

- Result 0: return without changes.
- Result 2: request sound 1 first. After that service completes, remove descendants,
  then request render. After render completes, set cancellation callback 0x2093ed
  and effective delay 4 (original stores 5 then immediately decrements).
- Result 3: remove descendants, set random-iteration count 30, then request the
  random provider. This continuation remains unconverted and suspended.
- Every other nonzero result, including 4..255: remove descendants, request sound 0,
  then render. After render completes, set global word 0x4545 to 4, OR bit 7 into
  global byte 0x454b, set acceptance callback 0x2094cb and effective delay 29
  (original stores 30 then immediately decrements).

Parent.result is never cleared by these wait transitions. Parent.byte+19 remains
untouched. Callback identity does not change until the render service finishes.

A returned service request is **not service execution**. The caller must execute
that service successfully before invoking `ntm_course_parent_resume`. A stored
native phase prevents duplicate entry/cleanup. Do not advance the dispatcher
while suspended, or reinvoke the unchanged original-style wait callback. The
service adapter must preserve task state except for the service's specified
side effects. Sound/render code, asynchronous failure handling and real-time
pacing are not implemented by this module. RANDOM cannot be resumed yet; it
returns an explicit error unchanged rather than skipping its animation.

The added native task.delay is a state-level 16-bit value, not the original binary
object layout. None of these APIs claims TLCS register, stack or cycle ABI parity.

## Verification

- **131,328 original/native allocation cases**: every priority/stale-result byte
  pair, both ordinary and circular insertion, plus 256 exhausted-pool cases.
  Full RAM and original returned task/cursor are checked.
- **66,304 original/native parent cases**: 768 startup cases (every priority and
  zero/one/two free nodes) and 65,536 wait cases (every result and parent priority).
  Mixed recyclable/nonrecyclable descendants and global state are included.
  Each sound/render/random boundary is checked before service stubbing. Ordinary
  accept/cancel then resume and compare final state. All non-stack RAM is checked;
  exactly 16 bytes of original stack scratch below entry SP are exempted.
- Parent services are observed/skipped in isolated original tests. Result 3 stops
  before the random provider; its later animation is not validated as native.
- Independent native tests cover 65,536 allocation sequences and 131,072 parent
  sequences, including cleanup, same-call delays and rejected duplicate resumes.
  Host, ASan/UBSan, freestanding m68020 compilation and QEMU m68020 execution pass.
- Five live original traces contain **194 allocation calls**, all successful:
  navigation 22, cancel 24, Option+A 22, fairway 61 and O.B. 65. Native allocation
  predictions match. Original video/audio/final state and fresh repeats are exact
  over 12,000 display frames. These traces also retain prior input, selector and
  lifecycle checks. **Parent begin/resume is tested in isolated cases, not a live
  parent observer or replacement game loop.** Live exhaustion is not observed.
- Running allocation/parent selftests before navigation leaves its complete trace
  byte-identical to the same run without selftests.

No new stock target timing was measured for this slice. Previous rendering and
selector/lifecycle timing artifacts remain preserved at their earlier source
checkpoints. QEMU unit tests are functional 68k tests, not stock-Amiga throughput
measurements. PAL A1200, 2 MiB Chip and zero Fast remain the preferred target; no
expanded-machine result is substituted for stock evidence.

Reproduce with `make -C native-game allocation allocation-sanitize allocation-m68020
parent parent-sanitize parent-m68020`, `bash tools/build-reference.sh --parent`,
and `reference/run.py --allocation-cases --parent-cases --allocation-check` with
private authorized ROM/input arguments. Compare the five existing scripts with
`reference/compare_allocation_trace.py`. Public CI builds the oracle and runs
synthetic tests, not private cartridge/firmware tests.

## Next bounded transitions

The decoded original timers decrement modulo 65536 and continue when the result's
sign bit is set, not merely at zero. Cancellation later clears parent.byte+19 and
delay, then invokes a table-driven viewport helper until complete, reports 0xfc to
its owner, deletes descendants and completes. Acceptance later maps selection
0,1,2 to 2,0,1, calls additional unconverted setup services, resets viewport bounds,
reports 0 to its owner and completes. Modified confirmation performs 30 random
selection/render/sound 2 iterations before ordinary acceptance. These remain next
conversion steps; none is silently folded into this checkpoint.
