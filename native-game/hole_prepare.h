/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_HOLE_PREPARE_H
#define NTM_HOLE_PREPARE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint8_t descriptor[18];
    const uint8_t *pins;size_t pins_size; /* Sixteen little-endian x/y pairs. */
    uint8_t dimensions[2];
    uint16_t map_index,aux_index;
    uint32_t descriptor_identity,path_identity,aux_identity;
} ntm_hole_geometry;
typedef struct {
    const ntm_hole_geometry *holes;size_t hole_count; /* course*18 + hole */
    const uint8_t *hazards;size_t hazards_size; /* Ten-byte records, FFFF terminator. */
    const uint8_t *quarter;size_t quarter_size; /* Original fixed-point quarter-wave table. */
    const uint8_t *variation,*threshold,*wind_low,*wind_high,*wind_limit;
    size_t variation_size,threshold_size,wind_low_size,wind_high_size,wind_limit_size;
} ntm_hole_prepare_resources;
typedef struct {
    uint8_t players[2][64]; /* Existing player records, preserved gaps. */
    uint8_t hole[256]; /* Existing semantic hole-state record, original4200..42ff. */
    uint8_t auxiliary[17]; /* Existing auxiliary record, original4400..4410. */
    uint8_t order[36]; /* Existing (course,hole) pairs, original441d. */
    uint8_t difficulty,selected_difficulty,alternate,secondary_mode,ready_flag;
} ntm_hole_prepare_state;
/* Original204a77 and normal-mode prerequisites. Requires course/hole order
 * entry selected by hole[6] in0..17, both difficulty bytes0..7, mode bytes0.
 * Uses the shared RNG. Pointer identities are opaque provenance, not dereferenced.
 * Returns0 or -1; validates all caller resource spans before changing state. */
int ntm_hole_prepare(ntm_hole_prepare_state *,const ntm_hole_prepare_resources *,
    uint8_t *random_cursor,const uint8_t random_window[256]);
#endif
