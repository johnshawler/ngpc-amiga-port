/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_GOLFER_H
#define NTM_SHOT_GOLFER_H
#include "shot_scene_objects.h"
#include "shot_terrain.h"
#include "shot_projection.h"
typedef struct {uint16_t selector;ntm_screen_asset asset;} ntm_shot_golfer_asset;
typedef struct {
    ntm_palette_job palettes[3][2];
    const uint8_t *course_selectors,*ground_records,*foreground_records,*frames,*overrides,*green_records,*pin_records,*metadata;
    size_t course_selectors_size,ground_records_size,foreground_records_size,frames_size,overrides_size,green_records_size,pin_records_size,metadata_size;
    const ntm_shot_golfer_asset *assets;size_t asset_count;
    const ntm_shot_aim_resources *aim;
} ntm_shot_golfer_resources;
typedef struct {
    ntm_task_callback drawing;
    uint32_t delta_x,delta_y,sample_x,sample_y,direction_x,direction_y,depth_scratch;
    uint8_t attempts,surface,task_bytes[8],phase;
} ntm_shot_golfer;
typedef struct {
    ntm_shot_scene_object_context *scene;
    const ntm_terrain_resources *terrain;
    uint16_t *terrain_detail;
    uint8_t player_status;
} ntm_shot_golfer_context;
/* Exact20a58d..20a5a9 initializer, including real ahead-of-ball terrain
 * sampling, scenery/HUD stamps, palette queues, perspective and tag51 pin.
 * Resources are borrowed. The next20a5aa drawing callback is provided by shot_golfer_drawing.
 * A caller must bind that real adapter before advancing the scheduler. */
int ntm_shot_golfer_initialize(ntm_shot_golfer *,ntm_task *,ntm_shot_golfer_context *,const ntm_shot_golfer_resources *);
#endif
