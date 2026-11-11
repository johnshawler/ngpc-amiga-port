/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_BANNER_H
#define NTM_SHOT_BANNER_H
#include "tee_setup.h"
#include "shot_prepare.h"
#include "shot_raster.h"
typedef struct {
    ntm_palette_job palettes[2];
    ntm_screen_asset terrain,characters[6],players[2],tee_shot,portrait,hints[2];
    const uint8_t *metadata;size_t metadata_size;
} ntm_shot_banner_resources;
typedef struct {
    ntm_tee_setup_context *shared;ntm_hole_prepare_state *hole;ntm_shot_player_state *player;
    uint8_t controlling_player,suppress_sound;
    /* Mandatory verified animated-palette service for requests6c/70. */
    int (*exit_palette_request)(void *,uint16_t);void *exit_palette_context;
    ntm_shot_raster *raster;
} ntm_shot_banner_context;
enum {NTM_SHOT_BANNER_DELAY=1,NTM_SHOT_BANNER_ANIMATION_WAIT,NTM_SHOT_BANNER_HOLD,
    NTM_SHOT_BANNER_EXIT_WAIT,NTM_SHOT_BANNER_DONE};
typedef struct {
    ntm_task_callback delay,animation_wait,hold,exit_wait,notice_entry,animation_entry,exit_entry;
    uint8_t phase,blink_delay,blink_state,finish_outcome;
} ntm_shot_banner;
typedef struct {ntm_task_callback wait;uint16_t variant;uint8_t inactive,phase;} ntm_shot_banner_animation;
typedef struct {ntm_task_callback tick;uint8_t phase;} ntm_shot_banner_exit;
/* Bounded genuine initial tee: terrain4, stroke1, modifier0, character0..5,
 * player0..1. Other lies and input64's optional menu stop explicitly. */
int ntm_shot_banner_initialize(ntm_shot_banner *,ntm_task *,ntm_shot_banner_context *,const ntm_shot_banner_resources *);
int ntm_shot_banner_tick(ntm_shot_banner *,ntm_task *,ntm_shot_banner_context *,const ntm_shot_banner_resources *);
int ntm_shot_banner_notice(ntm_task *,ntm_shot_banner_context *);
int ntm_shot_banner_animation_initialize(ntm_shot_banner_animation *,ntm_task *,ntm_shot_banner_context *,const ntm_shot_banner_resources *);
int ntm_shot_banner_animation_tick(ntm_shot_banner_animation *,ntm_task *,ntm_shot_banner_context *,const ntm_shot_banner_resources *);
int ntm_shot_banner_exit_initialize(ntm_shot_banner_exit *,ntm_task *,ntm_shot_banner_context *,const ntm_shot_banner_resources *);
int ntm_shot_banner_exit_tick(ntm_shot_banner_exit *,ntm_task *,ntm_shot_banner_context *);
#endif
