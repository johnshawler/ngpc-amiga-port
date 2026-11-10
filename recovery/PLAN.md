# Neo Turf Masters recovery decision and reproducibility gates

## Verified surviving baseline

Repository: https://github.com/johnshawler/ngpc-amiga-port
Commit: 01c2da88c61f8ec3dcbd2a7c807b72b8e4946d74

The six-commit history contains AGA smoke-test source, emulator scripts/configs, game notes, screenshots and eight pinned reference submodules. There is one advertised branch and no tags/PR refs, releases or workflow runs. No runtime, translator, renderer, game snapshot or later optimization exists in that history.

The exact parent repository was cloned without initializing or executing third-party code. git fsck passed. A Git bundle was created and verified; a fresh clone from that bundle reproduced the same HEAD with a clean tracked worktree. This is a source restoration test, not a game build. Parent confirms broad Mac/Space searches found no current golf source archive; original cartridge input survives separately.

Critical caveat: amiga/cfg/a1200.fs-uae specifies fast_memory=8192 and audio_driver=null. It is not the later stock zero-Fast test configuration. The AGA source explicitly requests PAL, but emulator configuration alone does not constitute stock timing evidence. No current compiler, FS-UAE or Mednafen executable was located by this audit. Submodule gitlinks are preserved, their content has not yet been restored.

## Boundary requiring a decision

The latest working port has NOT been recovered. Earlier runtime speed/equivalence reports are historical results of missing artifacts, not reproducible results of this repository.

Continuing from this baseline means rebuilding the missing translator/runtime/renderer and validation infrastructure. That is material reimplementation and must be reported before starting. Do not recreate the former ngpc-amiga-lab path and present it as a restored checkpoint. Keep this audit and original Git baseline untouched; use a clearly named new recovery branch/workspace after the decision to proceed.

Do not infer original source files from summaries or treat partial snippets as a full recovery. Any later discovered original artifacts take precedence only after identity and completeness checks.

## Proposed sequence after proceeding is authorized

R0. Durable source recovery
- Establish an isolated recovery branch from the exact baseline, retain main unchanged.
- Store the audit and testable scripts on that branch, verify the remote commit independently.
- Record all eight submodule commit IDs and license notices; fetch only needed official dependencies first.
- Never run the old all-in-one setup script blindly. Its latest distro packages and make -k build are not reproducible proof of a successful compiler build.

R1. Inputs and tools
- Materialize the privately retained cartridge and Kickstart through their verified source. Check local bytes against a manifest; no commercial inputs in Git.
- Cartridge notes identify SHA-1 308361297bd28ebb8fb60713ae5e00cee514415c. Prior missing workspace recorded SHA-256 92c7b5a3d6d28f8918c030afb95f9e4c13038326b8391d7fb05feac26917051a. Recompute, do not assume either matches.
- Pin tool source, executable versions and build commands. Keep compiler failures fatal. Separate build prerequisites from running the game.
- Create independently labeled PAL A1200 stock and optional expanded configurations: 68EC020/AGA, 2 MiB Chip, zero Fast for stock. Any expanded run must explicitly state its Fast size, never above 8 MiB.
- Restore and rebuild the simple AGA test as a fresh smoke test, not a golf milestone.

R2. Independent original-game oracle
- Rebuild a pinned original NGPC core with deterministic clock/RTC/input control.
- Establish new reset-to-menu/first-hole scripted checkpoints, CPU/RAM/GPU state, audio events and live video captures.
- Recheck original update cadence and compare images frame-by-frame. Do not reuse unavailable hashes as expected outputs.
- Freeze this oracle before introducing translation/native replacements.

R3. Minimal native vertical slice
- Rebuild deterministic assets/graphics extraction and a minimal native renderer with tests against original pixels.
- Implement a clearly bounded game routine or scene transition with original-state/observable-event comparisons.
- Do not silently call a general emulator the requested native game port.
- Quantify stock execution speed, memory footprint, stack and audio correctness. Retain an oracle/fallback until replacement entry/exit contracts are covered.

R4. Native gameplay and audio regions
- Prioritize game-level and sound-state routines, timed observable writes and event boundaries.
- Historical profiling suggests sound execution and runtime scheduling dominate; remeasure on new code before optimizing.
- Replace stages incrementally with interrupt, mutable-code, partial-budget and negative tests.
- Run complete CPU/RAM/GPU/native audio comparisons on representative scenes and a long reproducible reset path. Then perform stock boot/timing tests for each composed candidate.
- A sound-control rewrite or timing surrogate is not a complete game; do not claim playability or full speed until actual interactive play is demonstrated.

## Checkpoint acceptance and durability

Every meaningful checkpoint must include:
1. Exact source commit and dependency pins, build commands and test commands.
2. Input identity manifest (hashes/size, not commercial bytes in public or unspecified repositories).
3. Explicit target configuration and measured results with limitations.
4. Machine-readable test results linked to the tested executable hash.
5. Clean-room restore/build/test check in a different directory or worker.
6. Verified remote source commit before saying source is backed up.
7. Private artifact archive for any necessary non-Git material, stored through an authorized durable destination, with its verified identity recorded.
8. A manifest separating source-only/public-safe materials from private cartridge-derived artifacts, snapshots and ROMs.

A tarball beside the working directory is NOT durable backup. A bundle of the parent Git repository does NOT include submodule source or ignored runtime artifacts. Git submodule pins alone do not prove dependency content was fetched or recoverable. A passed short fixture is NOT a full-game or hardware timing pass.

Preserve the previous checkpoint before each change, never rewrite its result files. If a test is interrupted, mark it incomplete and resume only with identity-bound inputs. Avoid one huge ignored workspace containing the only copy of the port.

## Current status

Audit and local baseline bundle restoration complete. No new toolchain/emulator install, source reimplementation, publication or game build has been performed. The user directed continued recovery and development; R1-R4 may proceed on the independent recovery branch. Main and the missing old workspace path remain unchanged.
