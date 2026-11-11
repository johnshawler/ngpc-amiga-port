# Original-game gameplay coverage gates

These deterministic input scripts run the original NGPC game in the host oracle,
with the new native scene compositor checked at every visible scanline. They do
not run native game logic on the Amiga and do not measure Amiga speed.

Both scripts start with fresh temporary save storage and frozen reference time.
They select one-player stroke play, the default golfer and the Japan/Fuji course.
Inputs are two-frame A presses with release gaps; no external save is required.

| Script | Frames | Visually inspected outcome |
| --- | ---: | --- |
| `inputs.gameplay-ob.json` | 3600 | Hole 1 tee shot, ball flight, water splash, O.B. banner, third-shot setup |
| `inputs.gameplay-fairway.json` | 4200 | Hole 1 tee shot, ball flight, 268-yard drive, FAIRWAY banner, second-shot setup |

For the O.B. path, inspect frames 2340–2640 for flight/splash/O.B.; the fairway
path shows flight at 2400–2760, FAIRWAY at 3000, and second-shot setup at 3240–3600.
Frame numbers are zero-based. Local captures were inspected to establish these
labels; game images and derived hashes are deliberately excluded from source.

Each path passed every original raster-row comparison, an identical instrumented
repeat, and comparison with an unmodified reference run of all video/audio frame
outputs and final serialized state. The reusable trace validator rejects missing
rows, altered inputs/metadata, video/audio differences and final-state changes.

## Reproduce privately

Build the original and scene reference cores as in `SCENE_CHECK.md`. For each
script, use its frame count above and the same locally supplied cartridge:

    python3 reference/run.py --rom PRIVATE_GAME --inputs reference/inputs.gameplay-fairway.json --frames 4200 > build/original.jsonl
    python3 reference/run.py --core reference/build/scene/mednafen_ngp_libretro.so --rom PRIVATE_GAME --inputs reference/inputs.gameplay-fairway.json --frames 4200 --scene-check --capture-dir build/private-captures --capture-every 120 > build/observed.jsonl
    python3 reference/run.py --core reference/build/scene/mednafen_ngp_libretro.so --rom PRIVATE_GAME --inputs reference/inputs.gameplay-fairway.json --frames 4200 --scene-check > build/repeated.jsonl
    python3 reference/compare_scene_trace.py --original build/original.jsonl --observed build/observed.jsonl --repeated build/repeated.jsonl

Screenshot export uses portable RGB PPM, honors source RGB565 byte order and
rejects a conflicting existing screenshot. Captures, logs, save data and hashes
must remain private in ignored build directories. CI tests the public synthetic
trace validator and screenshot encoder, not the private cartridge runs.

## Remaining gaps

No putting, completed hole, bunker recovery, additional courses or multiplayer
coverage is established. These two outcomes are useful renderer cases, not full
game equivalence. The Amiga still has a static synthetic scene fixture; native
interactive game logic, audio and throughput remain unfinished.
