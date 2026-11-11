# Native course-selection consumer

`course_choice.c` translates the original three-country course-selection handler
into native state-level C. The original screen at the observed handler interval
shows Japan, U.S.A. and Germany. No original graphics or machine-code bytes are
included. Existing input/renderer checkpoints remain unchanged.

## Boundary and contract

The original handler starts at 0x209536, not the preceding three-byte mapping
table. It reads selection at 0x40fb, current/pressed buttons at 0x40bf/0x40c0 and
the B-disable gate at 0x4148. XIZ is its task object; [XIZ+12] points to its owner,
whose byte+17 receives the completion result.

The native state is just selection and owner result. Inputs are current buttons,
new presses and the gate. Its exact transition is:

1. When the gate is zero, B takes precedence: write result 2 and request finish.
2. Otherwise A writes result 3 when Option is held, result 1 otherwise, and requests
   finish. A takes precedence over navigation.
3. Left alone decrements, right alone increments. The arithmetic wraps as an
   eight-bit value, then accepts only values 0..2. Ordinary boundary moves are
   ignored, not wrapped. Both directions together do nothing.
4. Accepted movement updates selection and requests sound argument 2. Movement or
   inactivity preserves the owner result. Other direction/button bits do not
   affect navigation.

Synthetic out-of-range states retain the original behavior: selection 255 moving
right becomes 0; selection 3 moving left becomes 2. These cases are tested rather
than silently replaced with clamping.

The native API returns IDLE, MOVE_SOUND or FINISH. Sound and lifecycle execution
remain external dependencies. The original sound call is through ROM vector
0x250028 to 0x2501ba, with A=2; completion tail-jumps 0x20132b for task removal or
deferred removal. Neither backend is claimed to be native yet.

`ntm_course_choice_tick` composes the already verified native input-state update
with this consumer for one active-menu tick. The caller must own menu lifetime,
input mapping, scheduling, rendering and effect dispatch. Counting a FINISH
request in a benchmark does not implement the lifecycle transition. This is a
pair of native logic routines, not a playable course screen or complete golf loop.
The implementation uses no ROM access, CPU interpreter or allocation.

## Equivalence

The read-only original observer checks selection, owner result, sound-call target
and argument, and finish/normal-return outcome. Real sound and task services still
run in live traces. Isolated cases observe and skip only the sound call, preserving
the routine's two PUSH/POP pairs, and stop before lifecycle-helper side effects.
The oracle accounts for caller/stack scratch writes separately from game state.
RAM and architectural registers are restored before normal emulation resumes.

- 262,144 isolated consumer cases cover all 256 selections and 256 pressed masks,
  both Option states and both zero/nonzero gate classes.
- 4,096 additional original/native composed input→course ticks pass, for
  **266,240 original differential cases** total.
- Independent native tests check **33,619,968 transitions plus 65,536 composed
  ticks**. Host, ASan/UBSan and static QEMU m68020 pass. This is not an exhaustive
  enumeration of every task object, scheduler state or sound implementation.
- Five live traces contain **220 course-handler calls**, **6 movement-sound
  requests**, **3 normal confirms, 1 cancel and 1 Option+A confirm**. They cover
  12,000 original display frames and 6,000 verified input-state updates.

| Trace | Frames | Handler calls | Sound requests | Finish |
| --- | ---: | ---: | ---: | --- |
| Existing fairway gameplay | 4,200 | 44 | 0 | Normal confirm |
| Existing O.B. gameplay | 3,600 | 44 | 0 | Normal confirm |
| Added left/right and boundary navigation | 1,400 | 44 | 6 | Normal confirm |
| Added B cancel | 1,400 | 44 | 0 | Cancel |
| Added Option+A | 1,400 | 44 | 0 | Modified confirm |

All five preserve original video/audio/final serialized state and match fresh
instrumented repeats. The new navigation script returns the cursor to Japan,
with two rejected boundary moves and six accepted moves. Private original ROM,
captures, RAM traces and their hashes remain local. Public scripts contain only
button timing. State/effect equivalence is not a TLCS register/flag/cycle ABI.

## Stock-target comparison

The same target binary measures input-only versus the composed active-course
pipeline, with identical sample streams. Each of four variants runs 16,384 calls,
then repeats in reverse order. Gate 0/1 are both covered. Sound/finish requests
are counted, not dispatched; the synthetic benchmark continues invoking the
handler after FINISH specifically to measure repeated-call cost.

- Input-only in this binary: **129.8–130.2 microseconds/tick**.
- Input+course transition+request counting: **259.3–260.6 microseconds/tick**,
  approximately **0.260 ms**.

Final state bytes and request counts match an independent synthetic oracle.
Fresh-boot timing reports repeat identically. The profile remains stock PAL
A1200 emulation, 68020/AGA, 2 MiB Chip, zero Fast/Slow, cycle-exact CPU/memory,
full 320x256 eight-plane display DMA. Chained CIA timers calibrate against a
20.0246 ms PAL frame. This is repeated-call microbenchmark time, not a worst-case
cache-cold invocation, sound/rendering cost, whole-game FPS or physical-hardware
result. Compare the input baseline inside this binary, not a differently laid-out
older executable.

## Reproduction

```
make -C native-game course course-sanitize course-m68020
bash tools/build-reference.sh --course
python3 reference/run.py --core reference/build/course/mednafen_ngp_libretro.so \
  --rom private-inputs/neo-turf-masters.ngc --course-check --course-cases \
  --frames 4200 --inputs reference/inputs.gameplay-fairway.json
# Repeat both existing gameplay scripts and the three inputs.course-* scripts.
# Compare each against its original-core run with compare_course_trace.py.
bash native-renderer/amiga/build.sh --course-benchmark
bash native-renderer/amiga/run.sh --course-benchmark
python3 -m unittest discover -s tests -v
```

Use the existing documented cross-tool/emulator environment variables. Public CI
builds the oracle and runs no-ROM tests, not the private-game differential suite.
The separate `--consumer-probe` mode is diagnostic only and does not establish
routine equivalence. Next integration work is the native task lifecycle/dispatch
bridge and actual input adapter, keeping sound and rendering costs separately
measured.

## Follow-on lifecycle integration

`task_lifecycle.c` now provides a native course callback, completion/recycling and
one dispatch iteration. See [TASK_LIFECYCLE.md](TASK_LIFECYCLE.md) for the additional
contract, original differential coverage and separately measured target fixture.
The earlier isolated course API and benchmark above remain unchanged; allocation,
frame scheduling, rendering integration and real audio remain outside that API.
