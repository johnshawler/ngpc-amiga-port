# Native course and tournament title lifecycle

`course_title.c` continues the existing scenery object's `TITLE_PENDING` boundary
through original callbacks 0x20781a..0x2079c3. It does not replace the existing
intro ABI. The optional [round-record continuation](ROUND_RECORD.md) now handles
the normal fresh-record screen; tee-shot, shot and hole simulation remain outside
the converted boundary.
The original outer owner remains scheduled and requests round-record only on
its later visit after this child has actually been removed.

## Resources and order

All assets are caller-resolved, borrowed spans. The full course descriptor has
12 bytes. Its first four bytes have already been consumed by the scenery stage.
The title consumes the next words as title-stamp tile, object tile and object
animation index. The tournament consumes the final word as its stamp tile.
The title stamp is group entry +12 at map offset 0x188; tournament normal/mode 2
stamps are group entries +8/+16 at offset 0x180. The five-byte object metadata
comes from original 0x21980e. No cartridge bytes or derived assets are embedded.

The title object has tag 38, position (40, 48) plus the two shared scroll words,
and its descriptor-selected animation index replaces metadata width. It is
registered before applying that final position/index. Registration appends the
id/second word at the current list byte offset, increments offset by2 and count
by 1, marks the list dirty, sets restart, clears byte 30 and sets link 65535.
A full list (count>=32) drops registration including the latter three writes;
remaining initializer writes still happen. Rendering arbitrary multiple active
objects is outside the separate title object's current rendering contract.
The metadata velocity is preserved, not zeroed by registration.

Initialization stores delay 59 and returns without decrement. On subsequent
wait callbacks, a successful skip advances without decrement; otherwise the
16-bit timer decrements modulo 65536 and advances when its result is negative.
The tournament transition resets display state, directly clears the 12x8 map
region at0x188, then queues its selected stamp. Existing queued work remains
queued and can subsequently overwrite that clear. Queue saturation drops the
new request, not a retry. This reset only clears low bytes of the shared scroll
words, preserving their high bytes. The short delay stores 2 and immediately
decrements to 1. Expiry opens the full viewport and stores 59 without decrement.

The skip helper matches original 0x205318. In mode 0 it tests bit 0x10 of the pressed
control byte at40c0. In nonzero mode it ORs two caller-resolved linked control
bytes and tests the same bit. The original linked addresses are obtained from
two record pointers at41c9/41cd, their word at+0x31, then byte+1. The lifecycle
core has no access to original addresses; an adapter choosing linked mode must
supply actual resolved values rather than silently substituting normal input.

Final expiry/skip closes the viewport, resets display state, removes descendants
and removes/recycles the task using the existing verified lifecycle helpers.
The owner result is unchanged. COMPLETE is stable on repeat calls and does not
rerun cleanup. The separate application keeps the title object distinct from the selector
controller so later selector synchronization cannot overwrite its state.

## Title object

The existing object service also accepts the bounded title tag 38 with resource
indices 20/24/28 and animation selector 0. Each course has one component, containing
53, 61 or 59 sprites. On restart its duration 1 produces a first displayed frame.
The next update reaches header 0xc0 and holds that same frame: delay becomes 0,
flags gain 0x42, status gains 0x40, and no frame-advanced 0x80 status is added. This
expiry still requests a tile refresh. Subsequent frozen updates do not advance
or requeue tiles. Geometry, clipping, flips, hide/restart, queue saturation and
delayed double-buffered uploads preserve the selector service behavior.

## Verification

Cartridge-free unit tests cover all 196,608 states of the three timer callbacks,
all 65,536 pairs of linked input bytes, registration, repeated initialization,
cleanup with a descendant, stable completion and unchanged owner result.
Host, ASan/UBSan and static m68020 under QEMU pass.

The isolated original oracle uses an existing unchanged screen-probe core and
private cartridge. It covers 396 title initializations: all three courses, all 33
stamp queue occupancies, and object-list counts 0/1/31/32; 561 title-wait cases
include all four selected game-mode classes, signed timer boundaries, skip and
full tournament queues;144 later-delay/cleanup cases include both reserved and
recyclable tasks with/without descendants;2,304 original input-helper cases
cover all control bytes, normal/nonzero modes and both linked controls.
It compares full modeled non-stack RAM, callbacks, descriptor cursor, ordered
services, list registration, queue requests, task removal, maps and tile output.
The object oracle adds 768 cases / 6,144 tail-head rounds over all three courses,
all initial flag/delay bytes, empty/almost-full/full tile queues, flips, clipping,
hide, freeze/restart, forced refresh and removal. All modeled non-stack RAM,
8192 tile bytes, 256 sprite bytes and 64 palette indices match. A separate synthetic
unit test exhausts 196,608 flag/delay states and passes host, ASan/UBSan and m68020.
The unchanged selector object regression still passes 6,528 original rounds.

The integrated original scheduler comparison adds 12 flows / 1,470 updates across
all three courses, ordinary/random accepts, title/tournament skips and cancels.
It checks ordered tasks, free pool, object state, controls, records, tiles, maps,
sprites, palettes, queue state, viewport and sound requests after every update.
The owner resumes exactly one update after title-child cleanup and stops before
round-record allocation. Both default and private-source copy paths are checked.
A separate frozen-clock original/probe/repeat/raster run covers 1,800 frames
through title, tournament and later original-only screens. Video, audio and
final core serialization match across all four passes; the raster compositor
matches 273,600 rows / 43,776,000 pixels. This observes unchanged original
execution and validates the compositor, not later native gameplay.

These are bounded native state/service comparisons, not a whole-game native
replay, audio synthesis, physical-NGPC timing or evidence of playable golf.

```
make -C native-game title title-sanitize title-m68020
make -C native-game title-cursor title-cursor-sanitize title-cursor-m68020
python3 -m unittest discover -s tests -p 'test_course_title*.py' -v
# Requires existing reference/build/screen-probe and private cartridge:
bash tools/check-course-title.sh
```

Do not rebuild the shared reference core while other oracle runs are active.
All generated validation output stays under ignored build/course-title-check.
