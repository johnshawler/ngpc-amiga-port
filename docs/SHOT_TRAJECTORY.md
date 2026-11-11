# Bounded trajectory child

`native-game/shot_trajectory.c` converts original `20b3aa`, helper `20db22`, palette request `20db62`, and `20b432`/`20b49e` refresh. It initializes the actual tag27 slot16 object, registers it through the existing object-list engine, queues the original golfer palette and HUD asset, and installs the real waiting callback. The caller supplies label41dd, not club index41dc.

Refresh compares the label as a signed byte, with categories <=9, <=20, and greater. It preserves the saved resource bases, original wrapped camera addition, queue drops, and cached-label suppression. Initialization sets cached label255, so label255 does not draw on that call. Lifecycle4 returns a boundary before `20b43f`: no swing animation, lifecycle mutation, task completion, or gameplay has been invented.

Resources are borrowed spans resolved privately from the user's cartridge. `reference/shot_trajectory_case_common.py` resolves metadata, helper tables, six palette requests, and eighteen genuine group00ac HUD assets. No cartridge data or extracted art is tracked.

## Shared renderer support

The parent ended the source freeze and authorized integrating the validated tag27 support into `tee_objects.c`. `tools/check-shot-trajectory.sh` now compiles an unmodified copy of the shared sources in its separate build directory. The global animation capacity remains40. Tag27 requires width60 and exactly these eighteen original selections: six multiples of64 from64 through384, plus0,8,16. Both animation lookups use dense index `3*(velocity/64-1)+(velocity%64)/8` for tag27 only. All other tags retain their existing index. Callers must resolve the tag27 resource into these eighteen dense entries, not index by velocity/4. The object record itself retains the original selection unchanged.

`reference/shot_trajectory_object_assets.py` provides the reusable private resolver and dense index mapping. `reference/compare_shot_trajectory_objects.py` resolves genuine shape/tile/animation spans and compares registration, tail and head against the unchanged original, including RAM, queue identities, sprite buffers and video. Frozen lifecycle1 selections, horizontal/vertical flips and clipped coordinates are covered. It does not claim lifecycle4 animation support.

## Checks

Run `tools/check-shot-trajectory.sh`; source the local m68k toolchain activation first to include static m68020 QEMU. It runs host and ASan/UBSan unit checks, both normal and private-screen original RAM/resource matrices (3,072 cases each), and genuine object-engine rounds (648 each). LeakSanitizer is disabled because this environment's ptrace prevents it, while address and undefined-behavior sanitizers remain enabled.

The child has not been integrated into the main application. Shared renderer support is integrated; application integration still requires supplying the corresponding real tag27 asset bundle. Main scheduling and parent lifecycle4 behavior remain outside this deliverable.

## Resource budget

The eighteen reachable sprite animations total1,584 bytes; their shared shape span is4,060 and tile span8,112, totaling13,756. The eighteen HUD map descriptors total1,310 bytes. Their one shared tile table requires the maximum20,736-byte prefix, not the last descriptor's shorter prefix. Existing club-highlight resources use the same table through3,392 bytes, making17,344 additional HUD tile bytes when deduplicated with that resident table. Together these are32,410 incremental raw art bytes, plus17 helper/metadata bytes and12 palette-offset bytes. Palette colors can share the existing global table; otherwise they require a432-byte prefix. Structs, alignment, code and packaging are additional. These raw counts do not establish compressed DD fit or authorize integration.

The dense-lookup library also passed the existing tee-object regression (282 cases,5,468 tail/head rounds) and initial-aim-scene regression (537 cases,11,779 rounds). These retain the original40-entry resource ABI and original indexing for every other tag.

After shared integration, the unchanged old-family corpora passed again: tee282 cases/5,468 rounds; initial aim537 cases/11,779 rounds; tag5 course/hole markers1,080 cases/21,600 rounds across all three indices and eighteen holes. The trajectory RAM oracle also checks native display/object tag-mirror synchronization after full-list registration drops.
