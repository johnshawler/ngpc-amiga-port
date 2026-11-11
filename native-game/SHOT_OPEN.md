# First visible aim scene

`shot_open_application` is an optional native continuation from original
`209830` through the first `2098af` viewport-opening update. It installs the
real next callback `2098ee`, with delay 3 after the original same-call decrement,
and then declares `NTM_SHOT_BOUNDARY_AIM_OPEN`. This is a visible, initialized
scene, not a playable hole, swing, ball simulation, or completion of a shot.

## Converted behavior

- `209830` removes the real panorama object, restores the world scroll words,
  and runs `209f5c`, `20a170`, `20a214`, and `20ab9f`. These register the ball,
  pin, miniature markers and power gauges and preserve four HUD map rows.
  Optional other-player ball registration keeps the original tee-box branch.
- Scene objects retain their original metadata, animations, shape components,
  sprite/palette banks and deferred tile jobs. The object engine supports the
  real tags 1–4 and 6–11. No representative art replaces original resources.
- The parent allocates all four real children in the original order. In normal
  mode, `20c744` immediately takes its original task-finish branch. Linked mode
  uses its actual countdown. `20c883` initializes its actual camera countdown.
- `20ace7` initializes club limits and executes the real `20adcb` redraw,
  including label/digit/lie stamps, power limits, HUD scratch and gauge position.
  The final digit queue-full return leaves a high register byte that affects
  `20da24`: its lie record offset increases by `0x300`. Native code preserves
  this original observable behavior using separate resolved spans.
- `20aee1` registers both direction markers and uses original quarter-wave
  lookup and signed fixed-point multiplication. Its required early ticks
  preserve dirty state, angle limits and marker positions.
- The two-update parent wait and opening are unchanged. All remaining tasks
  and the real palette/object tail finish in the opening scheduler update.
  No child completion, RNG value, elapsed update or drawing job is fabricated.

## Application and resource contract

Zero-initialize `ntm_shot_open_application`. Start `base.base` with
`ntm_shot_application_start`, enable `base` with
`ntm_shot_aim_application_enable`, and then call
`ntm_shot_open_application_enable` before the first update. Continue calling
`ntm_shot_application_update` on `base.base`. The coherent application uses
original normal player-selection code, whose active player is bank 0.

The old NULL-continuation path still stops at `209830` without running it.
Earlier shot-only targets retain their original boundary and link dependencies.

All resources are borrowed and remain alive until deferred consumers drain:

- Scene metadata for tags 1–11, the signed power-scale lookup window and decimal
  remainder table.
- Fourteen-club power records for twelve supported golfer/modifier combinations,
  fifteen terrain tables per combination, and separate queue-overflow spans.
- Exact club labels, eleven digit/blank assets, nineteen lie icons, quarter-wave
  data and both marker descriptors.
- The unchanged resolved scene object asset union.

`reference/shot_scene_objects_case_common.py`,
`reference/shot_club_case_common.py` and
`reference/shot_open_application_case_common.py` resolve those inputs privately.
There are no cartridge bytes, extracted art or original captures in source.
Runtime arithmetic is at most 32 bits.

## Explicit remaining boundaries

The application stops after opening, before the next `2098ee` dispatch.
The later ready gate, input handlers, active camera/hint logic, club-change
highlight/control paths, marker swing-state transition and shot physics remain
unconverted. The input-handler identities installed at opening are represented
as owned state; this bounded application never dispatches them. Unsupported
child paths return an explicit boundary instead of pretending they completed.

## Verification

Cartridge-free checks:

```
make -C native-game shot-scene-objects shot-scene-objects-sanitize shot-scene-objects-m68020
make -C native-game shot-club shot-club-sanitize shot-club-m68020
make -C native-game shot-markers shot-markers-sanitize shot-markers-m68020
make -C native-game shot-open shot-open-sanitize shot-open-m68020
python3 -m unittest tests.test_shot_open tests.test_shot_aim tests.test_shot_setup
```

The public fixtures cover 4,096 power-limit cases, 630 club cases and 24 authored
application flows, including cancellation, repeated skips, earlier unsupported
branches, and preservation of the old opt-in behavior. The same application
runs under host sanitizers and an m68020 build.

Private unchanged-original gates:

```
tools/check-shot-open.sh
```

- 1,024 isolated scene-producer cases compare all nonstack RAM, including
  saturated registration, other-player paths, wrapped coordinates and HUD power.
- 540 club-initialization cases compare all nonstack RAM and exact queued asset
  identities across all twelve power tables, fifteen terrains and queue limits.
- Both the ordinary and private Amiga asset-copy backends pass 24 integrated
  flows and 7,473 updates, covering all three courses and six golfers. Each
  reports 21 first-visible boundaries and three cancellation outcomes. Checks
  include real scheduler callbacks, task/free links, input, shared RNG,
  hole/player/aim state, child scratch, maps, tiles, sprites, palettes and
  pending jobs. The 21 separate drawing-head drains per backend test pending
  consumers without advancing a frame or dispatching the boundary callback.
- Marker children independently pass 1,536 full nonstack RAM cases, 5,193 signed
  multiplication cases and 8,192 direction lookups. A further 8,731 comparisons
  cover exact delay/lifecycle behavior, including all 255 nonzero hint modes,
  7,932 countdown ticks and 512 genuine mode-0 task-finish outcomes.
- The object engine independently passes 537 cases and 11,779 tail/head rounds,
  including every new scene tag and the optional other-player ball slot.

Target RAM/disk fit, display cadence and stock A1200 measurements belong to the
separate Amiga frontend verification. Passing these source checks alone does
not establish full-game speed.
