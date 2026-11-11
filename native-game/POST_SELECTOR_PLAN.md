# Next native gameplay-flow boundary

The first scenery prefix described here is now implemented and tested in
[COURSE_INTRO.md](COURSE_INTRO.md). The course-title object, tournament
transition and cleanup are now converted
and independently verified in [COURSE_TITLE.md](COURSE_TITLE.md). This document
retains the original boundary research. Round-record and tee-shot setup remain
future work.

Read-only original execution identifies the sequence after course acceptance:
course scenery and title, tournament title, round-record screen, then tee-shot
setup. The preserved selector-only target ends at selection; the separate intro
target continues through the title lifecycle and stops at the round-record
request. A fast selector does not imply later game stages are implemented.

## Bounded next conversion

The outer owner at original callback `0x203f31` waits for result `0xf9`. Result
zero invokes helper `0x212d64`, allocates course-intro child `0x20771d`, and changes
its own continuation to `0x203f59`. Other results return to prior selection.
The parent exit does not immediately rerun the owner; preserve task traversal.

The normal helper clears six per-golfer flags, selects the active player's golfer
record, increments its 16-bit count and marks it active. A special record value
six invokes a separate unconverted helper. Reject or explicitly request that
branch until implemented; do not silently apply the normal path.

The smallest meaningful visual continuation is the intro initializer through
`0x207809`: clear scrolling and one map region, reset owned object/display state,
queue course-specific palette and background jobs, wait through the original
short delay, then open the full viewport. Existing palette/map services can be
reused after differential tests establish direct-versus-deferred clear ordering.
Assets must remain caller-resolved spans supplied privately from the cartridge.

Later title work adds another stamp, a registered animated title object and
input-skip/delay behavior. The owner subsequently allocates round-card child
`0x21480a` and gameplay setup child `0x208753`. They are separate conversion
steps, not completed by the scenery initializer.

## Verification gates

- All owner result bytes, both player-record banks and six valid golfer indices.
- Count wrap boundaries and explicit special-helper handling.
- All three mapped courses, occupied queues and nonzero prior display state.
- Signed 16-bit delay boundaries and no duplicate increments/allocation on resume.
- Original/native non-stack state, service order, tile/map/palette output and
  first-open-viewport pixels.
- Frozen-clock integration against unchanged original execution, followed by
  host/sanitizer/m68020 and stock PAL target tests.

The initial investigation used live original observations and isolated original
routine probes. It establishes the next boundaries, not native equivalence,
physical NGPC timing, or whole-core noninterference. Private captures and trace
output remain excluded from this repository. No later gameplay or audio-speed
claim follows from it.
