# Golfer drawing tag 52

The real initial drawing `20a9ce` dispatches through `21b989`. Its first
registering handler chooses animation word 20; the second chooses word 16.
These are animation offsets, distinct from dispatch-table offsets 0 and 4.
The next two handlers choose `ffff`, so the common code returns before
registration; the last dispatch is a return. The threshold loop advances the
pointer from `21b971` before its first comparison at `21b975`.

The metadata at `219854` selects index 60, initial animation 16 and plane 2.
Registration uses slot 18, base `49bb`, tag 52 and tile `0145`. The final
animation word replaces the metadata default after registration. The existing
object renderer already implements both genuine animations without a new
parser, synthetic frame, increased capacity or changed layout. Its validator
now accepts exactly tag 52/index 60/words 16 and 20.

`reference/shot_golfer_drawing_object_assets.py` resolves both single-frame,
single-component animations from the unchanged private cartridge. Their
borrowed spans total 12,764 bytes: 4,220 shape-prefix bytes, 8,528 tile-prefix
bytes and two 8-byte animation descriptors. Sharing prefixes with tag 27 and
tag 51 reduces the increment to 232 bytes (56 shapes, 160 tiles, 16 animation).
These are raw data counts, excluding code, structures and alignment. They do
not establish fit in the current stock target or DD image. The freestanding
GCC m68020 renderer object grows from 5,032 to 5,068 text bytes with no
data/BSS growth; this is not a linked target-size measurement.

Run `tools/check-shot-golfer-drawing-objects.sh` with the local cross toolchain
activated. It checks all 65,536 animation words and all 65,536 resource indices
on host, ASan/UBSan and static m68020/QEMU, and compiles the renderer freestanding
for m68020. The private oracle runs 450 real registration/tail/head rounds for
each ordinary/private-copy build, including both animations, freeze, hide,
flips, clipping and wrapped coordinates. It compares full nonstack game RAM,
hardware/video bytes and borrowed-resource identities using the unchanged
original core. Contract fixtures are authored formats, not extracted art.

This source support is not an application/exporter integration, a playable
swing or a full-speed claim. It does not regenerate a target or modify the
original ROM/core. Initial drawing producer coverage is separate.
