/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_SCENE_OBJECTS_H
#define NTM_SHOT_SCENE_OBJECTS_H
#include "tee_setup.h"
#include "shot_aim.h"
typedef struct {
    const uint8_t *metadata;size_t metadata_size; /* Eleven five-byte records, tags1..11. */
    const uint8_t *power_scale;size_t power_scale_size; /* Signed-byte window centred128 before2217bb. */
    const uint8_t *remainders;size_t remainders_size; /* Ten entries at21b82e. */
} ntm_shot_scene_object_resources;
typedef struct {
    ntm_tee_setup_context *shared;
    ntm_hole_prepare_state *hole;
    const ntm_shot_player_state *player;
    ntm_shot_aim *aim;
    uint8_t *saved_map; /*32 bytes, original44ff..451e. */
} ntm_shot_scene_object_context;
/* Exact209f5c/20a170/20a214 producers. They register real object records and
 * leave rendering to the existing queued object engine. Saturation preserves
 * original registration/drop behavior. No scheduler or shared RNG changes. */
int ntm_shot_scene_ball(ntm_shot_scene_object_context *,const ntm_shot_scene_object_resources *);
int ntm_shot_scene_pin(ntm_shot_scene_object_context *,const ntm_shot_scene_object_resources *);
int ntm_shot_scene_gauges(ntm_shot_scene_object_context *,const ntm_shot_scene_object_resources *);
/*20b764/20dba8 and20a373. */
int ntm_shot_scene_power_limit(ntm_hole_prepare_state *,const ntm_shot_player_state *,
    uint8_t club,const ntm_shot_scene_object_resources *);
int ntm_shot_scene_power_position(ntm_shot_scene_object_context *);
/*20ab9f saves four eight-byte map rows without changing any drawing queue. */
int ntm_shot_scene_save_map(ntm_shot_scene_object_context *);
/*209830 through209869, before the four genuine child allocations. */
int ntm_shot_scene_objects_initialize(ntm_shot_scene_object_context *,
    const ntm_shot_scene_object_resources *,const uint8_t world[52]);
#endif
