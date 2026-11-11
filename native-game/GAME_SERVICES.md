# Native viewport, setup, queued-object and exit services

This checkpoint converts four additional game services and connects viewport
state to the native planar compositor. It preserves the previous recovery
checkpoints. It is not a playable game or a recovery of the former lost runtime.

## Converted services

- **Viewport fade, original 0x2013a1.** Read X and width using two independently
  signed byte indices into a caller-supplied 256-byte private asset window. The
  second index increments with byte wrap before sign extension. Increment the
  task's substate by two modulo 256 and return busy while the new value is below
  14. Y/height remain unchanged. Normal cancellation takes seven updates.
- **Course setup, original 0x2052ad.** For valid mapped courses 0..2, reset the
  course index and write the cyclic order c,(c+1)%3,(c+2)%3. Other fields remain
  unchanged. Invalid mapped inputs are rejected; the original unchecked table
  lookup outside the valid domain is not emulated.
- **Queued-object initialization, original 0x2037a8 for WA=0x54 only.** Find the
  first slot with its sign bit set. Legal low bytes 0,4,...28 select native records.
  Initialize the selected record according to unsigned mode<16 versus mode>=16,
  using nine caller-supplied private descriptor bytes. Preserve unlisted fields.
  Mark the slot/category and status. Full queue returns a nonfatal full result
  without writes; the parent proceeds exactly as the original does. This is not
  conversion of every category or the consumer that ultimately renders objects.
- **Display/object reset, original 0x2006cf.** Reset the owned flags, 32 object IDs
  and tags, 32-entry list, two 256-byte pools, limits and fixed initialization
  words. Preserve unmodeled object data and the gap between the original pools.
  It neither removes task nodes nor resets the queued-object records itself.

Native models contain owned fields rather than a 16 KiB emulated RAM image.
Objects, queues and reset state are caller-owned. The original wire layouts,
little-endian descriptors and scattered addresses are adapted by the reference
oracle; they are not native struct ABIs. Private assets remain external inputs,
not published arrays. Distinct callback identities and coherent task graphs are
required, as in the existing lifecycle contract.

## Parent integration

The parent now resumes through the converted fade, setup, queued-object service
and reset. Acceptance writes its final viewport state before exit. Cancellation
and acceptance deliver owner results 0xfc and 0 respectively, delete descendants,
then perform the already verified task unlink/recycle. Full queues do not prevent
completion. Pending services cannot be replayed by advancing the tick function.

Complete ordinary accept, cancel and modified accept take 32, 13 and 92 parent
callback invocations respectively in the controlled tests. Sound and the
course-screen drawing service at 0x209603 are still observed/stubbed in those
flows. Therefore this does not demonstrate a visible interactive menu, native
audio, shot physics or a complete game loop. Actual caller/renderer scheduling
and the queued-object consumer remain to be connected.

## Rendering connection

`ntm_scene_set_viewport` applies the converted four-byte viewport to the native
compositor's row-latched scene. The existing compositor and packed converter then
produce AGA-format planar pixels. The caller must apply state at the correct
latch boundary; this does not infer a frame scheduler or replace palette/audio
work. Existing caches already include/read viewport state in their contracts.

Independent synthetic tests cover 256 complete viewport geometries, including
clipping outside the visible region, and check every output pen and planar bit.
This bridge has also executed with the real native compositor on stock A1200
emulation, using authored synthetic assets and an offscreen Chip bitmap.

## Verification

**85,088 original/native differential cases pass:**

- 65,536 fade cases: every substate byte across varied vertical/register inputs.
- 768 valid course-setup cases across varied RAM seeds.
- 18,432 queued-object cases: every mode, first-free/full position and legal slot
  permutation, preserving all unrelated fields.
- 256 reset cases with varied initial RAM.
- 96 complete parent exit flows covering all three result classes, both full/free
  queues, both initializer modes and recycle-flag classes. The original fade,
  setup, queue and reset services actually execute. Sound/render remain stubs.

All non-stack RAM and viewport outputs are compared; only the known 16-byte stack
scratch region is excluded. Carry results are checked for the standalone fade
and queue services. Synthetic tasks were relocated away from reset's actual RAM
pools before executing the real reset. RAM, registers and the entire GPU state
are restored afterward. Running all seven differential suites before navigation
leaves its trace identical to a run without them.

Independent native tests cover 102,400 fade/queue cases, 256 complete planar
fixtures and 1,536 complete parent exits. Host, ASan/UBSan and m68020 execution pass.

Five live original traces match native predictions at **236 service returns**:
112 fades, 4 setups, 4 queued-object initializations and 116 resets. Original
video/audio/final serialized state and fresh repeats remain exact across 12,000
display frames. These live checks observe the original game; they do not replace
its entire execution with native game code.

## Stock target result and remaining bottleneck

The new fixture runs native fade→scene viewport→cached composition→packed planar
conversion on an emulated stock PAL A1200: 68020, 2 MiB Chip, zero Fast, full
eight-plane display DMA. CIA calibration is 20.026 ms. Two seven-step authored
viewport sequences match an independent pixel/depth oracle and all planar output
hashes. A fresh run repeats the timing report byte for byte.

Full redraw costs **1,829–2,093 ms** across those synthetic window sizes. The
first stage includes a cold sprite cache. This is still nowhere near a 20 ms PAL
budget. It is an offscreen component measurement, not game FPS or physical
hardware timing. The static display remains active; sound and actual course-screen
drawing are not included. Correct viewport integration is not acceleration.

A practical next rendering step is to update the newly hidden strips of an
existing bitmap using Amiga masked fills, rather than recomposing the entire
unchanged scene during a shrinking viewport. That must be validated against the
full redraw and measured before claiming improvement. Course-screen drawing and
real audio remain separate implementation work.

Reproduce with `make -C native-game services services-sanitize services-m68020`,
`bash tools/build-reference.sh --parent`, and `reference/run.py` flags
`--service-cases --service-check` with the private inputs. Use
`reference/compare_service_trace.py` for the five representative scripts. Target
build/run use `--viewport-benchmark`. Public CI runs source/oracle builds and
synthetic tests, without cartridge or firmware inputs.
