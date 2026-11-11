# Native course-introduction scenery

This is the first bounded continuation after course selection, not a playable
hole. `course_intro.c` converts the outer owner's accepted-result path, the
normal golfer-record helper, and the scenery initializer through the first open
viewport. `course_intro_application.c` integrates it with the existing selector.
Title animation, tournament title, round record and tee-shot gameplay remain
explicitly unconverted.

## Contracts and order

- The outer owner is a real scheduled task. A selector exit writes its result
  but does not call the owner again in the same traversal. On the next update,
  result zero updates the selected golfer's record and allocates the intro child;
  that child runs later in the same traversal. Result 249 waits. All other result
  bytes stop at the prior-selection request.
- The normal helper selects bank 0 only when the player byte is zero; every
  nonzero byte selects bank 1. It clears six active flags, increments the chosen
  golfer's 16-bit count modulo 65536, then marks that golfer active. Only indices
  0 through 5 are supported. Record kind 6 returns a special-helper request
  before mutation. Acknowledging that request is not permission to skip it:
  `special_done` is valid only after a caller actually implements its effects.
- Intro initialization zeros both plane scroll offsets and directly clears the
  20 by 19 map-0 area, before display reset. This clear is not deferred. Existing
  fill and stamp queues are preserved, so older queued jobs can write there on
  the following drawing head. Owned display fields reset; tile memory, hardware
  sprite memory and unrelated object fields are not indiscriminately erased.
- The shared palette manager receives the two mapped-course jobs, then the
  existing stamp queue receives the background at map-1 offset 0x800. Full queues
  silently drop their individual requests, matching the source. They are not
  retried on later updates.
- The initializer stores delay 2 and immediately decrements to 1. Later ticks
  decrement to 0, then open the full 160 by 152 viewport and initialize the next
  delay to 20 with its immediate decrement to 19. All delay subtraction is
  modulo 65536 with a signed-result test, including 0 and 0x8000 boundaries.
- Expiry of the visible delay sets the title continuation and returns a stable
  title request. The wrapper stops there. The owner's next child-wait completion
  would request round-record, but no later screen is manufactured or skipped.

The wrapper preserves input, palette head, old fill/stamp drains, object head,
ordered tasks, palette tail and object tail. After display reset the empty
object list uses the usual tail/head path to remove the stale selector sprite.
The original selector application remains the default; its driver has an
optional scheduled owner hook, unused by the unchanged selector target.

## Private resources

The native code receives three caller-resolved resource groups, never a ROM
pointer. Each group has a borrowed descriptor span, a resolved background
map/tile span and two palette jobs. It retains the descriptor cursor after its
first two words for future title work. Validation and queue consumption follow
the existing screen/palette contracts. The caller keeps these spans alive until
all queued work has consumed them.

`tools/export-course-intro-assets.py` resolves the private resources for the
separate target. No original assets, hashes, screenshots, saves or captures are
tracked. A fresh application's record state is normal golfer zero in bank zero;
it does not import or represent restored save progression.

The separate target can opt into `NTM_SCREEN_PRIVATE_ASSETS`. That contract
requires source asset spans not to alias destination tile/map arrays. It uses a
bounded 16-byte copy backend without changing validation, queue order or writes.
The generic default producer/consumer remains available. Both paths have been
compared with the original; target timing belongs to the target report, not the
host results below.

## Verification

Cartridge-free native tests cover all owner result bytes, all 256 record-bank
selector bytes, all six golfers and six count boundaries; invalid/special record
contracts; occupied queues; repeat requests; allocation exhaustion; real task
traversal; and all **131,072** input states of the two 16-bit delay callbacks.
Host, ASan/UBSan and static QEMU m68020 pass. `tests/test_course_intro.py` exposes
these checks to the existing public unittest CI without a cartridge.

Private original-routine comparisons cover:

- **108** record cases: both banks, an additional nonzero bank byte, all six
  golfers and six count boundaries. Special helper entry is checked separately.
- **512** owner cases: all 256 result bytes with a populated and empty free pool.
- **891** scenery cases: all three courses crossed with all nine palette queue
  occupancies and all 33 stamp queue occupancies, randomized prior fills,
  palette/staging state, maps, tiles and other RAM.
- **40** original signed-delay boundary cases, complementing exhaustive native
  delay tests.
- **12 integrated flows / 1,065 updates**: six ordinary accepts, three
  modified/random accepts and three cancels, covering all three mapped courses.

The isolated oracle compares the entire modeled non-stack RAM plus all map,
character and palette bytes at initialization and each service boundary. It
checks exact clear/reset/palette/stamp call order. Integrated comparisons check
native/original task links, callbacks, allocation pool, published task, controls,
cursor objects, records, tiles, maps, sprites, palette queues, viewport and sound
request events after each logical update. It specifically asserts the one-update
selector-to-owner delay and the two-update initializer-to-open delay. Both default
and private-source copy paths use the same oracle. Sound events are not audio
synthesis equivalence.

A separate **1,300-frame, three-pass** frozen-clock run of the unchanged original,
probe-enabled original and repeated probe original matches every video/audio
frame and final core serialization. Each process uses the runner's fresh save
directory. This verifies that the observation build does not alter those live
runs; it is separate from controlled native routine comparisons. The controlled
scheduler comparison stops before unsupported title/prior-selection instructions.
It is not a whole-game native replay or a physical-NGPC timing claim.

A separate original-raster compositor check covers **197,600 rows / 31,616,000
pixels** over the same 1,300-frame input sequence, including the first open
course scenery, with no native/original raster mismatch and unchanged original
video/audio/final serialization. It validates the existing compositor against
the original GPU at its actual draw point, separately from the controlled native
task/state comparison.

## Reproduce

```
make -C native-game intro intro-sanitize intro-m68020
python3 -m unittest discover -s tests -p 'test_course_intro*.py' -v
bash tools/check-course-intro.sh
# Optional fresh live noninterference and original-raster checks:
bash tools/build-reference.sh
bash tools/build-reference.sh --scene
python3 reference/check_course_intro_reference.py --scene
```

Cross tools must be on PATH. The oracle requires the pinned original reference
source and the user's cartridge at `private-inputs/neo-turf-masters.ngc`.
All generated artifacts remain under ignored `build/course-intro-check`.
Do not rebuild the reference core concurrently with reference runs.

See the separate [intro target report](../native-renderer/amiga/intro_README.md)
for stock PAL target display, capture, timing and pacing results. No success in
this bounded slice means that title animation, round-record or tee-shot routines
have been converted. The next title object uses a different tag and animation
format, including flags unsupported by the selector cursor engine; do not reuse
that engine without extending and verifying those dependencies.
