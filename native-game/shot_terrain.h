/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_TERRAIN_H
#define NTM_SHOT_TERRAIN_H
#include <stddef.h>
#include <stdint.h>
typedef struct {const uint8_t *data;size_t size;} ntm_terrain_map;
typedef struct {
    ntm_terrain_map large,close;
    const uint8_t *masks,*shapes;size_t masks_size,shapes_size;
} ntm_terrain_resources;
typedef struct {uint16_t x,y,detail;uint8_t terrain;} ntm_terrain_result;
/* Original20fb18,2050f3,201782,20fbbd. Preserve scratch/detail on early
 * branches; fail without mutation if a selected resource is out of bounds.
 * hole is the existing256-byte hole state. Resources are borrowed spans. */
int ntm_shot_terrain(const uint8_t hole[256],uint8_t player_status,
    uint16_t x,uint16_t y,const ntm_terrain_resources *resources,
    ntm_terrain_result *result);
int ntm_terrain_map_word(const ntm_terrain_map *map,uint16_t x,uint16_t y,
    uint16_t *word);
#endif
