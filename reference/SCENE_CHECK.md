# Original raster-point scene comparison

`bash tools/build-reference.sh --scene` makes a separate reference core from a
fresh export of the pinned upstream source. It inserts a read-only comparison
immediately after the original renderer draws each visible scanline, before RGB
conversion. The submodule and original/instrumented build modes stay unchanged.

The adapter passes the actual latched scrolling, window, sprite, palette and
character state to the new native compositor. It resolves pens through the
current original palette, including inversion, and compares all 160 RGB12 values.
Exported counters record compared rows/pixels, mismatches and unsupported rows.
No frame-boundary approximation and no asset capture is involved.

Run locally with a privately supplied cartridge:

    python3 reference/run.py --core reference/build/scene/mednafen_ngp_libretro.so --rom PRIVATE_GAME --frames 600 --inputs reference/inputs.example.json --scene-check

The command rejects any mismatch, unsupported row or zero-row run. Logs contain
private-input hashes: keep them under ignored `build/`, never publish them.
Run the same inputs with the unmodified core and compare every frame's existing
video/audio fields and final serialized state. Repeat the instrumented run.
This checks that observation leaves emulation behavior unchanged.

The example input sequence is startup/menu coverage, not a played golf hole.
A passing comparison proves only rows actually visited. CI builds the adapter
and checks no-cartridge metadata; it cannot establish game equivalence without
the private cartridge. The comparator is host test infrastructure, not a target
CPU emulator or the Amiga game runtime.
