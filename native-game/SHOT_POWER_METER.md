# Native power meter

`shot_power_meter` translates original child entry `20b70b`, callback `20b74b`,
its oscillator `20b7c8`, and tile-map renderer `20b865`. It reuses verified
`20b764`/`20dba8` power-limit and `20a373` marker-position helpers and the
`2004e2` indexed tile copy. It is an opt-in module, not a playable-shot claim.

Initialization clears `427c` and `427d`, sets position 256, lower bound 1 and
hold 1, calculates the exact table-driven speed, stores the upper index, loads
nine real glyph assets, and installs the supplied native callback adapter.
It does not execute the callback immediately. Scratch mapping is task+22:
position word, +24 velocity word, +26 upper byte, +27 lower byte, +28 hold byte.

The speed is the wrapped sum of words read at signed, doubled-byte offsets
from the original player and lie speed tables. Lie 6 with zero player modifier
doubles the result. Borrowed windows encompass all signed-byte offsets; they
are resolved privately, never embedded in the source. The initializer loads
nine indexed graphics assets at tile-memory offsets 0xe40 through 0xf40.

Only lifecycle 2 advances the meter. Every other lifecycle calls the real task
finish helper, preserving priority deferral, unlinking, recycling, and cursor
movement. The oscillator compares the signed high byte of the wrapped candidate
position against signed bounds. Upper comparison is inclusive; lower comparison
is inclusive. Boundary reversal first saves bound*256 plus old velocity and
negates velocity, so the eventual advancement lands exactly on the bound.
Character 2 additionally sets hold 5, subtracts 256 from the stored position,
and publishes the upper lookup value. The hold byte always decrements modulo
256, including zero becoming 255. A zero result is restored to 1, position
advances, and the signed high-byte scale lookup updates power.

Rendering writes twelve pairs of tile-map words, upward from offset 0x300,
using full pairs, one remainder pair when space remains, and empty pairs. It
then updates the existing marker position. Corrupt power greater than 120 is
an explicit boundary before any native state mutation rather than an unsafe
out-of-bounds reproduction. No ball flight or invented power physics is used.
The separate entry `20b8c4` is a different meter implementation, not a successor
of this child's callback; it remains outside this module.

`tools/check-shot-power-meter.sh` runs authored host, ASan/UBSan and m68020/QEMU
checks and the unchanged-original comparator. The latter compares full nonstack
work RAM, task scratch/header, hardware and video memory. Only original measured
stack writes are excluded. No call interception or original-core modification
is used. Resources and reusable ctypes layouts live in
`reference/shot_power_meter_case_common.py`, separately from test execution.

Verification includes 31,741 authored rendering/hold/lifecycle cases on each of
host, sanitizers and m68020/QEMU, plus 5,373 unchanged-original comparisons:
512 initializers, 2,048 full-width oscillator states, 2,048 active callbacks and
765 completion cases covering every other lifecycle value and all three task
completion outcomes. No original ROM assets are included in these files.
