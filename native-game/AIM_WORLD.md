# Native aiming world loader

`aim_world.c` translates original `201493`, `20151a`, `2015c0`, and `20170e` into bounded C. It is the world-loading service called by `209e28`. It does not advance the aiming scene, run the later scrolling update `201608`, or implement shot input, ball flight, or audio.

## State and resources

- The caller supplies origin words `5710/5712` and the existing map/palette destinations. The loader clears exactly `5714..573b` and preserves the tail. Normal round initialization selects map base `9800`, monochrome palette destination `8110`, and color palette destination `8300`.
- The 52-byte state image includes the preserved first two bytes of the fill queue at `5742/5743`. These are an alias, not a new independently owned queue. Callers exporting the image must keep that alias synchronized or export only through `5741`.
- Resource spans are borrowed and bounded. Each has an explicit zero-offset position so signed resource offsets can read a valid prefix. The original bank/record identities retained in four state fields are diagnostic metadata and are never dereferenced as addresses.
- A resolver must preserve the palette-bank byte from the original record and select the corresponding command stream. It must supply the actual layout and block windows, including reachable prefix bytes, rather than assuming the nominal width and height define all reads.
- Color hardware writes go directly to the live palette and background control. Palette staging, copy descriptors, pending controls, and job slots remain untouched.

The test resolver in `reference/aim_world_case_common.py` copies broad windows for isolated host comparisons. It is not an Amiga asset package: an Amiga exporter should share one bounded resource allocation among layout/block/index spans rather than duplicate a window per record. No ROM or copyrighted assets are included in source.

## Preserved behavior

The map routine rounds the origin down to 32 pixels, visits seven by seven blocks beginning one block above/left, and copies each four-by-four tile block into the circular hardware map. Coordinates and offsets retain original signed 16-bit and wrap behavior. The final scratch coordinates are the rounded origin plus 192.

Only the low byte of the tile count controls copying. The original can write beyond character RAM, including a real 209-tile record. The translation validates the selected source indices, copies the 208 tiles that fit beginning at offset `1300`, and ignores later writes to the original unmapped destination.

Color command offset zero is a real color group. Negative entries skip the corresponding destination group; they do not clear it. Monochrome commands scan through the first negative sentinel and then process two four-byte groups. Unmapped transparent-color registers ignore writes. Background control retains only its readable `c7` mask.

All selected source spans and destinations validate before any mutable output changes. Invalid or missing data returns `-1`, not a fabricated successful load. No 64-bit runtime arithmetic is used.

## Verification

`tools/check-aim-world.sh` runs:

- Cartridge-free checks of 65,536 wrapped origin pairs, both map layers, exact color/monochrome branches, tile-count behavior, and atomic malformed-span rejection.
- The same checks with AddressSanitizer/UndefinedBehaviorSanitizer and big-endian 68020 execution.
- 864 unchanged-original runs covering all 108 map records, both map layers, and mode values 0, 15, 16, and 255.
- 468 additional unchanged-original coordinate cases, including negative origins, 31/32 pixel boundaries, and signed wrap extremes.

Original comparisons cover all 16 KiB of non-stack game RAM, including actual queues, and the complete 16 KiB hardware/video range. Only actual original stack scratch is excluded. The test adapter accounts for the reference core's first monochrome scroll-palette readback being shifted one byte relative to its write addresses; the native interface retains the hardware write layout.

The stock-target freestanding 68020 object is 1,644 bytes with only `memcpy` and `memset` imports. This object-size and unit-test result is not a frame-rate or complete-game claim. Integration and actual A1200 display timing remain the calling application's responsibility.

## Private resource budget estimate

A deduplicated union of bounded resource ranges, preserving the full signed-offset envelope for all 54 large maps, measures 290,600 bytes. Of these, 824 bytes overlap the existing shot asset pool, leaving 289,776 new asset bytes. The existing 231,082-byte pool would become 520,858 bytes, excluding the unchanged 256 assembled palette bytes. Resource structures are 102 bytes each on 68020, or 5,508 bytes for 54 records.

For comparison only, retaining nominal map records without the necessary prefix/read envelopes would add 227,184 bytes. That is a lower-bound size estimate, not a safe export plan. The three first-hole records would add 220,918 bytes with the full envelope, or 37,982 bytes using that unsafe nominal-only bound. No resources have been removed to meet a disk-size target.

These resource-byte estimates exclude new initialization code, alignment outside the pools, executable relocation overhead, disk allocation overhead, and any further scene assets. They are not an exact disk-fit result. A future package needs verified shared bounds and/or a packaging change; the existing single-DD image's remaining capacity does not accommodate even the nominal three-hole estimate unchanged.

A size-only compression experiment on the complete 520,858-byte shared pool produced 445,081 bytes using the repository's existing PackBits encoder, and 208,726 bytes using zlib level 6. Both streams round-tripped exactly. Relative to the current uncompressed 231,082-byte pool, those are respectively 213,999 more bytes and 22,356 fewer bytes, before any new metadata, decompressor, relocation or disk-block overhead. Nothing was implemented in the target, and startup cost, live memory use, and final disk fit were not measured by this experiment.
