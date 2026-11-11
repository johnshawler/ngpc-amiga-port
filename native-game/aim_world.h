/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_AIM_WORLD_H
#define NTM_AIM_WORLD_H
#include <stddef.h>
#include <stdint.h>
/* A bounded borrowed window. origin is the position corresponding to source
 * offset zero. Prefix bytes are necessary for the original signed offsets. */
typedef struct {const uint8_t *data;size_t size,origin;} ntm_aim_world_span;
typedef struct {
    ntm_aim_world_span layout,blocks,tile_indices,tiles,commands,colors,mono;
    /* Diagnostic/source identities only; never dereferenced as addresses. */
    uint32_t layout_tag,blocks_tag,terrain_tag,tiles_tag;
    uint8_t width,height;
} ntm_aim_world_resources;
typedef struct {uint8_t bytes[52];} ntm_aim_world_state;
/* Original201493,20151a,2015c0 and20170e. bytes is the5710..5743
 * image (5742/5743 alias the first fill-queue word and are preserved).
 * Caller supplies origin5710/5712, map base573c (9000 or9800), and
 * mono/color palette destinations573e/5740. Clears exactly5714..573b.
 * Color mode is the original40fa byte: >=16 selects color hardware.
 * Direct hardware writes update tiles/maps/palette/background, NOT palette
 * staging or queues. mono represents8100..8117, may be NULL in color mode.
 * All selected spans/destinations validate before any mutation. Return0 on
 * success,-1 for missing/malformed resources or unsupported destinations.
 * Spans and mutable buffers must be disjoint. No cartridge runtime required. */
int ntm_aim_world_load(ntm_aim_world_state *,const ntm_aim_world_resources *,
    uint8_t color_mode,uint8_t tiles[8192],uint8_t maps[4096],
    uint8_t palette[512],uint8_t mono[24],uint8_t *background);
#endif
