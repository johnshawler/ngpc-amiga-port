# Independent native-port rebuild

This branch starts from surviving setup commit 01c2da88. It does not contain or recover the missing later golf runtime. Historical frame rates and long regression passes do not apply to this rebuild.

Checkpoint R0:
- Original main branch, configuration and screenshots retained unchanged.
- Separate PAL A1200 configuration explicitly requests 2 MiB Chip and zero Fast; not yet boot-tested.
- Source integrity checks validate the baseline, pinned dependencies and stock configuration and reject tracked commercial/generated runtime inputs.
- Cartridge validation rejects wrong size and wrong identity; the real private cartridge has not yet been checked here.
- Three Python checks pass. Original reference-core rebuild is in progress separately.

No game executable, playable port, stock speed result or full-speed feasibility is claimed.

Run:
python3 -m unittest discover -s tests -v
python3 tools/recovery-check.py
python3 tools/recovery-check.py --rom private-inputs/neo-turf-masters.ngc

Keep commercial cartridge, Kickstart and derived snapshots/assets under ignored private-inputs or build directories. Source manifests and reproducible generators belong in Git; private files require a separate authorized durable store.

Checkpoint R1:
- Original cartridge privately materialized and matches historical SHA-1/SHA-256; Kickstart separately retained.
- Pinned original NGPC core built twice with identical hashes on the recorded toolchain.
- Deterministic frozen-RTC/input harness implemented; five source/no-ROM checks pass.
- Two independent 600-frame real-cartridge runs match all video/audio hashes and final state.
- 120-frame save/restore replay matches video and final state but differs on 16 PCM frames. Strict replay fails as intended; no complete replay pass is claimed.
- Source-safe results: reference/checkpoint-neo-turf.json. Private ROMs and output states are not committed.
- CI builds the pinned original reference and runs only source/no-cartridge checks. Real-cartridge checks remain local/private.
- No Amiga game implementation or speed claim.
