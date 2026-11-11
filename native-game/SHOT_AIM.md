# Initial scene and aim-state setup

The optional `shot_aim_application` continuation implements original `209751`
through `20982f`. It installs the real next callback `209830` and stops before
that callback executes. This initializes scene data, initial direction and club;
it does **not** implement aiming input, a swing, ball physics or a playable hole.
The viewport remains closed at this boundary. Deferred art is still queued.

## Converted behavior

- `20d5ca`: exact signed-16 coordinate comparisons, wrapped differences,
  integer ratio and borrowed angle table. The short-distance parent path keeps
  the original four-bit fixed-point scaling before extracting coordinates.
- `20d8fe`, `20d9a3`, `2050e9`: thirteen club thresholds, wrapped midpoint,
  golfer/terrain minimum, signed-byte indexing, and the actual tee-box check.
  The twelve power tables are resolved resources for six golfers and their
  supported modifiers; no ROM address is dereferenced in the runtime.
- `204e6c`: direction-dependent camera offset, signed bounds and wrapped origin.
- `209751`: exact scratch resets, initial-angle copies and explicit green-branch
  boundary. The first-tee application reaches only the normal terrain-4 path.
- `209e28`: genuine world loader, tile and map copies, direct palette/background
  updates and flag bit 3. See [AIM_WORLD.md](AIM_WORLD.md).
- `2037a8` command `0c`: real existing queued palette service, followed by the
  original HUD initialization and clear. See [SHOT_HUD.md](SHOT_HUD.md).
- `209e3a`: genuine deferred panorama request, object registration, tag 5 and
  the subsequent course/hole resource selection. All 54 course/hole combinations
  are represented. The existing verified object engine executes the actual tail.

The shared RNG is untouched by this setup. No waits, task completion signals,
child allocation or art queues are replaced with success stubs.

## Application contract

Use zero-initialized `ntm_shot_aim_application`. Start its `base` using the
existing `ntm_shot_application_start`, then call
`ntm_shot_aim_application_enable` before the first update. Pass the same base to
`ntm_shot_application_update`. All resources remain borrowed until consumers
have finished with the queued work.

The opt-in initializer supplies the genuine fresh-game map/palette destinations
written by original `2049eb`: map `9800`, monochrome `8110`, color `8300`.
Existing selector-only fixtures did not execute that earlier initializer; the
new original comparison explicitly seeds those same pre-entry values. The
world state exported by the application owns `5710..5741`. The loader's final
two preserved bytes alias the first fill-queue descriptor at `5742`, which
remains queue-owned and is not exported as an independent state value.

`NTM_SHOT_BOUNDARY_AIM_SETUP` means the `209830` callback is installed but has
not run. Return value 1 is a declared boundary, never completion of a shot.
`NTM_SHOT_BOUNDARY_UNSUPPORTED` remains explicit for unsupported earlier paths.
A NULL `aim_continuation` preserves the earlier first-tee/banner boundary at
`209751`; old targets have no link dependency on these new modules.

## Verification

Public, cartridge-free checks:

```
make -C native-game shot-aim shot-aim-sanitize shot-aim-m68020
python3 -m unittest tests.test_shot_aim tests.test_shot_setup
```

The helper fixture exercises 100,000 angle vectors, 131,072 tee-boundary checks,
6,144 club selections and invalid/green branches. The opt-in application fixture
runs 24 authored flows through real existing task, palette and object services,
including cancel, repeated skips and unsupported branches. All runtime arithmetic
is at most 32-bit. The freestanding m68020 aim-helper object has no imports.

Private unchanged-original comparisons use the user's existing local cartridge
and isolated original core, never rebuilding or patching that core:

```
tools/check-shot-aim.sh
```

- 4,321 direction vectors, 4,096 tee-box checks, 4,096 camera states,
  12,288 club choices and 4,096 setup states compare complete nonstack RAM.
- 162 panorama producers compare complete nonstack RAM, exact queued asset
  identities and real object registration, including saturated queues.
- 24 integrated flows and 7,410 updates compare tasks/free links, input, shared
  RNG, players/hole/aim/world state, HUD scratch, real services, maps, tiles,
  sprites and palettes. All three courses and six golfers are exercised.
- Live stamp identities/offsets/tile attributes are compared before the final
  boundary. A separate test-only head drain validates actual pending consumers
  without dispatching `209830`, changing inputs or pretending a frame advanced.

The bundle tests ordinary and private Amiga asset-copy backends. The world,
HUD and tag-5 object services also have their separate unchanged-original gates.
No extracted art, cartridge bytes or captures belong in version control.

This source checkpoint does not establish target memory/disk fit, A1200 display
cadence or game speed. The previously tested target remains unchanged.

## Optional next stage

The separately linked `shot_open_application` can now supply the genuine
`209830` continuation. Its explicit opt-in does not change the NULL/default
setup boundary described above. See [SHOT_OPEN.md](SHOT_OPEN.md) for the visible
scene, real child services, verification and remaining unconverted boundaries.
