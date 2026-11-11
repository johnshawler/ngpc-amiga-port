# Native normal hole overview

`tee_setup.c` converts the first tee scheduler's normal hole-overview child
0x20ca45, its real nested children 0x20ce7c and 0x20cf21, and the outer
0x208753/0x20876f scheduling through the next child request at 0x208785.
`ntm_tee_setup_application_start` is a separate opt-in application. It continues
selector, scenery, title, round preparation and record through the overview,
then stops **before allocating unconverted 0x2096a4**. There is no native shot
scene, aiming, swing, ball physics, hole completion or playable golf yet.
The existing selector, scenery, title and round application starts retain their
old boundaries. An optional callback in the base application dispatches the
new continuation on the real outer owner's later traversal.

## State and resources

The accepted path is normal game mode 0, with a valid selected course and hole
from the existing random course order. [HOLE_PREPARE.md](HOLE_PREPARE.md)
implements the actual 0x204a77 prerequisites and consumes the same shared RNG
cursor as the selector and record preparation. The application preserves the
selector's course order bytes, existing player records, hole state and relevant
auxiliary state. It does not choose a convenient hole or reset the RNG.

The caller supplies all 54 hole geometry and art groups, 18 ordinal labels,
three par labels, the direct header, indexed font tiles, palette jobs, four
object descriptors and object resources as borrowed, resolved spans. Native
code neither reads the cartridge nor executes original instructions. Source
pointer identities retained by hole preparation are opaque provenance only.
Resources must remain alive until every deferred stamp, palette and object
upload is consumed. The optional private adapter is source, not extracted art.

The new application defaults to independently observed fresh saved hole scores
of 10 and difficulty 2, with previous selected difficulty 0. Saved hole bytes
are distinct from the round screen's missing course-record flag of 255.
Caller state can be changed after start. Badge selection preserves original
byte arithmetic: saved score 1 selects offset 0; otherwise nonnegative signed
`score - par` yields no badge, and negative differences yield the wrapped
`(difference + 4) * 4`. Only offsets 0/4/8/12 and no-badge 255 are supported.
An invalid badge result stops at an explicit unsupported boundary after actual
preparation, rather than masking an invalid animation index or showing another
badge. It does not roll back already executed initializer effects.

## Original order and timers

The outer owner allocates the tee scheduler on the traversal after round child
cleanup. With skip byte 0, the scheduler allocates the overview later in that
same traversal. The overview issues service-1 sound-control requests 65535 and
65534 with parameter 8; clears shared and hardware scrolling; directly clears
map 0's 20 by 19 cells; clears text tile base; copies indexed font tiles directly;
submits two palette jobs; runs hole preparation; draws the header directly;
queues the ordinal label; draws a three-digit distance directly; queues the par
label; and allocates the first nested child. Full queues drop individual queued
requests without dropping direct writes.

For game mode 0, original 0x20df6a immediately clears carry. Its real 0x20ce7c
child therefore removes its descendants and itself on its first visit. The
parent resumes on its next scheduled visit, queues three hole views, resets the
owned display fields, registers three objects plus an optional fourth badge,
and applies pin and course-marker positions. Registration occurs before the
badge animation offset and final positions, matching the original.

The initial delay stores 2 and immediately decrements to 1. Expiry opens the
160 by 152 viewport and stores 4 with immediate decrement to 3. Its expiry
stores 179 without decrement. The next 180 ordinary hold visits complete the
overview; a pressed skip edge advances without decrementing that timer. On
completion it allocates child 0x20cf21. That child also immediately removes
itself in mode 0: its handicap display belongs to game mode 2, which is not
silently substituted here. Parent cleanup happens on the following traversal,
resets owned display state, closes the viewport, removes descendants and
recycles/removes the parent. The tee scheduler resumes one traversal later.

Both small delays and the hold use unsigned 16-bit wrapping subtraction with
an explicit sign-bit test. Initializer fallthrough and delay-store timing are
part of the contract, not elapsed-time approximations. Cleanup and the next
child request are stable and do not repeat on later calls.

The nonzero skip-byte branch omits the overview exactly as the original does.
It does not clear the player-status byte, whereas the completed-overview branch
does. The common music helper compares the actual current-music control byte
to the course. If different, it issues service-0 requests 65535/65534 with the
course parameter. The tee application's adapter preserves the small music-ID
metadata write performed by request 65535; all other audio work remains requests
only. A skipped overview needs caller-supplied current-music state, not a guessed
lookup table. This is not audio synthesis equivalence.

## Graphics and object state

`tee_graphics.c` implements the full-count indexed-tile copy and one-to-four
place decimal map output. It preserves scratch bytes beyond the requested
places, replaces leading zeroes with blanks, retains the least-significant
requested digits, and uses wrapped 16-bit tile/color arithmetic. Each operation
validates its complete bounded spans before writes.

[TEE_OBJECTS.md](TEE_OBJECTS.md) describes the independently verified
multiobject service. At the application transition, the old object 0 animation,
bounds, second sprite bank and both palette banks are preserved. The new display
reset changes IDs/tags only, preserving unowned object state. Head uploads,
map drains, task traversal and tail preparation keep their existing order.
Neither the scheduler nor the object service pauses to display a picture.

## Verification and limits

Cartridge-free checks include 458,752 complete timer states, 262,144 decimal
cases, all 65,536 badge byte pairs, every hole art selection, queue saturation,
owner branches, stable boundaries, invalid contracts and 30 complete synthetic
application flows. Host, ASan/UBSan and static QEMU m68020 runs exercise the same
authored native code.

The isolated original comparison covers 297 initializations, 162 all-hole/badge
setups, 256 later callbacks, four normal notices, 540 normal second-child/timer
cases, 512 owner entries and 18 owner waits. It additionally compares 504 decimal
cases and 768 badge cases. It checks full modeled non-stack RAM, original service
order, task links/free pool, direct/deferred maps and tiles, palette jobs, object
registration and cleanup. This isolated initializer intercepts preparation at
its explicit service boundary; preparation has its own independent oracle.
The integrated comparison covers 33 flows and 10,335 updates, including
ordinary and random accepts, cancellation, pressed skips, skipped overviews,
equal/different music state and optional four-object badges. It executes the
real native and original preparation,
all original nested tasks and the full shared drawing services without that
interception. Both generic and private-copy backends are checked.

Three additional malformed saved-score application regressions verify that actual
preparation consumes its six RNG draws before an unsupported badge offset stops
the driver without registering objects or queued art. Host, ASan/UBSan and
m68020 checks cover this failure boundary.

These are bounded semantic comparisons. Actual stock PAL A1200 display,
2 MiB Chip memory, disk packing, and performance evidence belong in the separate
target report. The earlier roughly 2.7-second full compositions are not full
speed. An overview target is a new target, not a retroactive expansion of the
old round target's measurements. No ROM, firmware, extracted assets or private
captures belong in tracked source.

```
make -C native-game tee-setup tee-setup-sanitize tee-setup-m68020
python3 -m unittest discover -s tests -p 'test_tee_setup.py' -v
bash tools/check-tee-setup.sh
```

The private check reuses the existing isolated original core. It must not
rebuild that core while reference work is running. Generated reports remain
under ignored `build/tee-check`.
