# Real golfer feature selection

`shot_golfer_features.c` converts complete original `21081d`, a prerequisite
called by the initial drawing stage `20a9ce`, now converted separately in
`shot_golfer_drawing.c`:

- `2100ca` collects at most 16 actual feature records in the original inclusive
  200-unit rectangle, preserving wrapped coordinates and original sign checks.
- `21082a` transforms them using the real direction, signed full-product
  arithmetic, fractional ball coordinates and original angle-table cone filter.
- `21098c` selects the nearest eligible record with the original tie order and
  signed comparisons.

The result retains both original 32-bit scratch values, both direction biases, complete
nearby/view list regions, and stale trailing bytes. Empty lists do not invent a
feature or clear scratch that the original leaves untouched. The final selected
record contains the real feature identifier, lateral coordinate and depth.

The borrowed current-hole list is resolved from hole offset `36`. Records have
one identifier byte and two little-endian coordinate words; byte 255 terminates
the list. No cartridge addresses are dereferenced by the target routine. Every
read that the native routine needs is checked before any caller state changes.
The original 16-entry cap can stop before a source terminator is read. The 54
actual lists total 5,464 raw bytes; the largest current-hole list is 216 bytes.
These figures exclude state, structures and code and are not a target-fit claim.

This helper does not draw scenery, register an object, select a replacement
callback or advance the scheduler. `shot_golfer_drawing.c` supplies the genuine art, map
shifting and tag 52 object path. The existing aiming target still stops before
fire, and no target artifacts or exporter were regenerated.

## Verification

`tools/check-shot-golfer-features.sh` passes 10,576 authored collection,
selection and arithmetic cases on host, ASan/UBSan and static m68020/QEMU.
Independent 64-bit product arithmetic appears only in the test fixture. The
native module uses bounded unsigned 32-bit arithmetic, not a 64-bit runtime.

The unchanged-original oracle passes 1,200 complete `21081d` calls:

- All 54 actual hole feature lists across eight genuine direction choices.
- Authored rectangle edges, signed/wrapped positions, fractional coordinates
  and full-width direction words.
- 758 visible selections, 48 empty nearby lists, and 168 capacity-16 lists.

Every comparison checks all nonstack game RAM and hardware/video bytes from
`8000` through `bfff`; only measured original stack writes are excluded. Original
instructions, resources and engine binaries remain unchanged. No private list
bytes, ROM hashes or trace captures are embedded in tracked source.
