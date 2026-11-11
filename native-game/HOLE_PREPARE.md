# Native normal hole preparation

`hole_prepare.c` converts original `0x204a77` and its normal-path prerequisites:
hole-order selection and geometry (`0x204b76`), player reset (`0x20d74b`),
difficulty selection (`0x204f57`), and wind (`0x204fa8`). The geometry path includes
pin selection, fixed-point scaling, terrain extents, auxiliary variation and
special-hole metadata. This prepares a hole; it does not implement a shot,
collision, ball flight, audio synthesis or rendering.

## State and resources

The supported contract is normal alternate/secondary mode zero, both difficulty
bytes in 0..7, an order index in 0..17, and a selected course/hole pair in
0..2 and 0..17 respectively. Unsupported inputs reject before state mutation.
The existing shared RNG cursor and signed-index window are passed explicitly.
There are exactly six draws, including the pin, auxiliary variation/direction,
and wind draws. The auxiliary variation uses the previous selected difficulty;
only later does normal difficulty selection copy the base difficulty.

The state contains the two existing 64-byte player records, the existing hole
record corresponding to `0x4200..0x42ff`, the auxiliary record at
`0x4400..0x4410`, and the course-order pairs. Bytes outside the original write
set are preserved. In particular, selecting pin coordinates replaces only the
high halves of the two 16.16 position words; their existing fractional halves
remain inputs to the following scale operation.

The caller supplies bounded, privately resolved geometry entries and tables.
Each geometry has its descriptor, sixteen pin offsets, terrain dimensions,
map indices and opaque source identities. Native code never dereferences those
source identities, fetches cartridge instructions or operates a CPU emulator.
The private reference adapter demonstrates descriptor-relative resolution,
18-byte map-entry strides and signed offsets to terrain dimensions. Art and
full terrain maps are not embedded in this preparation module.

Scaling by 5/8 uses exact 32-bit products and shifts, avoiding a runtime 64-bit
multiply dependency on m68020. The original short multiply (absolute input
below one fixed-point unit) truncates negative fractions toward zero; the full
signed multiply rounds negative fractions downward. Both paths are retained.
Direction values use the original borrowed
quarter-wave table with native index and sign arithmetic. The special-hole
record key is `(course << 8) | hole`; its overlapping word stores are preserved.
Wind retains byte wrap, negative clamp, per-order-index cap and final cap 15.
All resource spans must remain readable and disjoint from mutable state.

## Validation

- 110,592 cartridge-free exact-state cases cover all 54 geometry entries, every
  initial RNG cursor and all base difficulties, with varied prior difficulty,
  fractional positions, metadata matches, wrap, limits and untouched bytes.
- Invalid modes, indices, table spans and missing resources reject without
  changing state or the shared cursor.
- Host, ASan/UBSan and static QEMU m68020 runs pass. The freestanding m68020
  object has only the intended `ntm_random_next` unresolved symbol.
- 17,280 unchanged-original executions cover all 54 holes and all 256 RNG
  starts, plus all 8-by-8 difficulty combinations and all 18 order indices.
  Every modeled RAM byte is compared except the stack range actually used by
  each call (the lowest observed stack pointer is `0x6bbc`). Another 8,199
  arithmetic comparisons cover signed/fractional scale inputs and all 4,096
  direction angles. Prerequisite order
  and exactly six shared RNG draws are checked. Graphics memory is unchanged
  in sampled cases. This is a routine-level comparison, not whole-game replay.

Run `make -C native-game hole-prepare hole-prepare-sanitize hole-prepare-m68020`
with the existing toolchain. `bash tools/check-hole-prepare.sh` runs the private
original comparison using the already-built screen-probe core and authorized
cartridge. Generated reports remain under ignored `build/hole-prepare-check`.
Do not rebuild the shared reference core while other comparisons are running.
