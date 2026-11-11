# Native round preparation

`round_prepare.c` converts original `204a88` and its normal order prerequisites
`20dfe6`, `20d732`, `204aa5` and `21290b`. It prepares data; it does not implement
the round-record display, save loading, tee shot, hole simulation or other modes.
The supported contract is mapped course 0..2 and order kind at 0x4209 equal to 0.
Other order kinds reject before any mutation. The helper itself does not inspect
game mode at 0x41e9; the enclosing display/lifecycle must enforce its own mode bounds.

## Exact effects

- Clear round index at 0x4205 and hole index at 0x4206.
- For both 64-byte player records, clear offsets 0x26..0x2a and 0x35..0x37.
  Also clear offsets 0x2b and 0x30 unless reset kind at 0x4203 is 255,
  in which case preserve them.
- Set the 18 score bytes at offsets 0x10..0x21 in each record to 255. Other record
  bytes remain untouched except the selected course-record flag below.
- Order kind 0 selects **five distinct random holes**, not 18 sequential holes.
  Each draw consumes the application's existing shared RNG byte and takes its
  unsigned remainder modulo 18. Duplicate holes consume a draw and retry.
  Write five `(mapped_course, hole)` byte pairs to the first 10 order bytes;
  preserve all 26 bytes after them.
- Read the caller-resolved saved course flag corresponding to original
  `0x5847 + 66*mapped_course`, and store it at offset 0x34 of player 0 if the
  active-player byte is 0, otherwise player 1. Every nonzero bank byte chooses 1.
  Player offset 0x33, including the round display's record kind, is preserved.

The RNG resource is the same signed-index 256-byte window consumed by
`random_byte.c`. Callers must supply the existing shared cursor, not initialize
a private one. No cartridge or save bytes are embedded. The three saved flags
are caller state, not ROM resources and not implicitly assumed to be 255.

The original loops forever if its window has fewer than five distinct
remainders. The native boundary precomputes at most one complete 256-byte cursor
cycle and rejects that invalid resource without changing state or the cursor.
For accepted inputs the state and final RNG cursor match original execution.
Calling preparation again intentionally repeats the effects and consumes new
RNG draws; it is not an idempotent initialization guard.

## Validation

Cartridge-free tests compare complete state for 401,404 cases: every combination
of active-player byte, reset-kind byte and mapped course; all RNG starts with
six synthetic windows; repeated calls; preserved gaps/order tails; unsigned
modulo and cursor wrap; invalid pointers, course/order values and insufficiently
diverse windows. Host, ASan/UBSan and static m68020 QEMU runs pass.

The isolated unchanged-original oracle covers 8,556 calls, all 256 RNG starts,
all three courses, every active-player/reset-kind byte, every saved-flag value,
and repeats. It compares all 16 KiB modeled RAM except actual return-address
stack slots 0x6bd4..0x6be3, including untouched record gaps, the 26-byte order tail,
saved records and the shared RNG cursor. The four prerequisite entry calls
must occur in source order. It observes 47,532 RNG calls, with at most 9 draws
per preparation; graphics/register memory 0x8000..0xbfff is also unchanged over 108
repeated boundaries. This is a bounded routine oracle, not a whole-game replay.

```
make -C native-game round-prepare round-prepare-sanitize round-prepare-m68020
python3 -m unittest discover -s tests -p 'test_round_prepare.py' -v
# Needs the existing screen-probe core and private original cartridge:
bash tools/check-round-prepare.sh
```

Do not rebuild the reference core while other oracle runs are active. Generated
artifacts stay under ignored `build/round-prepare-check`.
