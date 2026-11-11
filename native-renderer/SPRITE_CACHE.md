# Resolved sprite cache

The optional `nr_scene_row_cached` path retains the original uncached compositor
as a baseline. It resolves relative sprite chains once per changed sprite state
and prepares a compact painter-order list for each visible row/priority. A sprite
appears in at most eight lists, so the complete entry pool is bounded to 512 bytes.
The full caller-owned cache occupies 2,144 bytes in the verified 68020 build.

Every active row compares all 256 sprite bytes, all 64 sprite-palette bytes and
the two global sprite offsets. Any change rebuilds the lists before drawing.
There is no assumption that sprite state stays fixed for a frame. Scroll maps,
character RAM, plane ordering and the window are still read live on every row.
The caller initializes the cache and never mutates its internal fields.

A cache hit skips all 64 coordinate-chain resolutions and the three 64-entry
priority scans. It renders only that row's prepared candidates. Cache rebuilding,
320-byte content checks, tile drawing and planar conversion still cost time;
a cache-hit rate is NOT a frame-rate measurement.

## Validation

The existing randomized and directed scene oracle now checks the cached path.
An additional 48,640 rows test changes to each individual descriptor, position
and sprite-palette byte. Host sanitizer and explicit m68020 QEMU tests pass.
The original-raster adapter compares both native paths on every visited row
before checking the original RGB12 pixels. The fairway and O.B. scripts are rerun
with original video/audio/final-state and instrumented-repeat comparison.

The separate `amiga/build.sh --cached` / `amiga/run.sh --cached` fixture uses the
same synthetic scene and independent expected pixels as `--scene`. It boots on
the same emulated stock PAL A1200, 2 MiB Chip, zero Fast configuration. Its Hunk
adds the cache as 2,144 bytes of BSS; the existing 100,000-byte graphics allocation
and previous source/artifact checkpoints remain separate and unchanged.

## Limits

This is a bounded renderer throughput optimization, not native game-logic
conversion. Hardware cycles and gameplay frame rate have not been measured.
The source trace's cache counters quantify avoided repeated work only. A target
benchmark of uncached versus cached drawing is still required to establish the
net speed benefit, including worst-case rapidly changing sprite state.
