# Native first-tee palette transitions

`shot_palette.c` adds a separate animated-palette sidecar to the existing
`ntm_course_palette` manager. Existing init-job APIs and layout stay unchanged.
The combined tail processes original kinds 4/32 and animated kinds 24/40 in
ascending original slot order, with the same eight slots and eight queued copy
descriptors. The existing head still uploads staging colors and pending controls.
The original palette kinds are not substituted with instantaneous black screens.

Original request 0x6c blends sixteen four-color groups at byte offset 256.
Only the three nontransparent colors of each group are recomputed; the staging
transparent color remains untouched, even though each upload contains four words.
Request 0x70 blends one background color at byte offset 486 and preserves the
unrelated control bits when queuing the pending background selection.

Source and target colors are resolved into borrowed contiguous spans by the
caller. The exact original interpolation table is a borrowed 512-byte window,
starting sixteen bytes before its nominal zero entry. It is not embedded in
native source. All address-derived resources are bounded before work begins.
The palette routines execute native color arithmetic and table lookups, not an
original CPU interpreter.

Each animated slot stores an unsigned eight-bit fractional accumulator and level.
Enqueue initializes level 255 and fraction zero. Addition wraps at eight bits;
a sum below sixteen does not enqueue any upload. Otherwise the low nibble stays
as the fraction and the high nibble advances the level, with wrapped addition
followed by a clamp to fifteen. At the actual speed sixteen, the first update
therefore advances 255 to zero; levels zero through fifteen produce sixteen
updates. The job frees its slot only after the level-fifteen upload is queued.
Input timing, task timers and head/tail ordering are not stretched for rendering.

Malformed resources, unsupported kinds, invalid slot permutations and copy-queue
overflow fail before either manager is mutated. An otherwise valid enqueue into
a full manager returns one without changing it. Resource lifetimes extend until
the job completes. The normal first-tee application must use this combined tail
once animated jobs can appear, rather than call the init-only tail on them.

## Verification

- 128 original cases and 5,120 tail/head rounds compare every modeled non-stack
  RAM byte, palette staging, copies, hardware colors and controls. Cases shuffle
  allocation order, mix original legacy groups with both animations, and use nine
  different accumulator speeds. The extended comparison also varies single-color
  endpoints, both supported controls and pending indices.
- 75,776 synthetic color/step states cover every 12-bit source color, all 256
  speed bytes, full queues, transparent-color preservation and atomic rejection.
- Host, ASan/UBSan and static m68020/QEMU tests are provided by
  `tools/check-shot-palette.sh`; public CI runs only authored synthetic fixtures.

This service is a prerequisite for the banner exit. It does not establish an
interactive shot, rendered landscape, audio synthesis or stock game frame rate.
