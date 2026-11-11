/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TEE_SETUP_H
#define NTM_TEE_SETUP_H
#include "course_intro.h"
#include "course_title.h"
#include "tee_objects.h"
#include "tee_graphics.h"
typedef struct {ntm_screen_asset views[3];uint16_t marker_x,marker_y;} ntm_tee_hole_art;
typedef struct {
    ntm_palette_job palettes[2];
    ntm_screen_asset font,header,hole_numbers[18],pars[3];
    ntm_screen_asset handicap[2]; /* Reserved mode2 resources, unused in mode0. */
    ntm_tee_hole_art holes[3][18];
    const uint8_t *metadata[4];size_t metadata_size[4];
    const ntm_tee_object_assets *objects;
} ntm_tee_setup_resources;
typedef struct ntm_tee_setup_context ntm_tee_setup_context;
struct ntm_tee_setup_context {
    ntm_course_intro_context *intro;
    ntm_task_context *tasks;ntm_task *fallback;
    ntm_course_cursor *objects;uint16_t *x;uint8_t *tiles;
    /* These are owned state, not resources. prepare must execute normal204a77
     * and then publish its actual outputs here. It may not fabricate a hole. */
    uint8_t game_mode,input_mode,control,linked_controls[2];
    uint8_t course,round_index,hole_index,par;
    uint16_t distance,pin_x,pin_y,origin_x,origin_y,text_base;
    const uint8_t *saved_hole_scores; /* row-major3*18, caller save state */
    const uint8_t *handicaps; /* Caller41ee state; normal child never reads it. */
    uint8_t reset_kind,language;
    int (*prepare)(void *,ntm_tee_setup_context *);void *prepare_context;
    /* service0 means original250020, service1 means250024, service2 means
     *25001c (WA command,C parameter). Requests only,
     * never a claim of native audio synthesis. Original init uses value8. */
    void (*sound_request)(void *,uint8_t,uint16_t,uint16_t);void *sound_context;
};
enum {NTM_TEE_NEW=0,NTM_TEE_NOTICE_WAIT,NTM_TEE_SETUP_DELAY,
      NTM_TEE_OPEN_DELAY,NTM_TEE_HOLD,NTM_TEE_HANDICAP_WAIT,NTM_TEE_DONE};
enum {NTM_TEE_NONE=0,NTM_TEE_COMPLETE=1,NTM_TEE_UNSUPPORTED=2};
typedef struct {
    ntm_task_callback notice_entry,notice_wait,setup_delay,open_delay,hold,
        handicap_entry,handicap_wait;
    uint16_t asset_group,asset_index;
    uint8_t phase,finish_outcome;
} ntm_tee_setup;
typedef struct {ntm_task_callback wait,cleanup;uint8_t phase,finish_outcome;} ntm_tee_handicap;
/* Exact2128c6 byte arithmetic after caller resolves saved byte and hole par. */
uint8_t ntm_tee_badge_offset(uint8_t saved,uint8_t par);
/* Normal game_mode0. Complete first child20ca45 with nested normal notice
 *20ce7c and handicap child20cf21. Other game modes reject before mutation.
 * Input edges remain scheduler-owned. Borrow resources until all queued work
 * has drained. It is a hole overview, not a playable tee shot. */
int ntm_tee_setup_initialize(ntm_tee_setup *,ntm_task *,ntm_tee_setup_context *,const ntm_tee_setup_resources *);
int ntm_tee_setup_tick(ntm_tee_setup *,ntm_task *,ntm_tee_setup_context *,const ntm_tee_setup_resources *);
int ntm_tee_notice(ntm_task *,ntm_tee_setup_context *);
int ntm_tee_handicap_initialize(ntm_tee_handicap *,ntm_task *,ntm_tee_setup_context *,const ntm_tee_setup_resources *);
int ntm_tee_handicap_tick(ntm_tee_handicap *,ntm_task *,ntm_tee_setup_context *);

typedef struct {
    ntm_task_callback overview_entry,overview_wait;
    uint8_t phase,skip_overview,player_status;
    uint8_t course,current_music; /* actual audio control byte6ad8 */
    void (*sound_request)(void *,uint8_t,uint16_t,uint16_t);void *sound_context;
} ntm_tee_owner;
/* Original208753/20876f/20534a to the actual unconverted2096a4 allocation
 * request. Phase2 is stable and cannot acknowledge/bypass the boundary.
 * A real child must complete before the zero-skip path clears player_status.
 * Service250208 compares course to current_music and preserves its input.
 * Requests do not pretend to implement the audio service's state changes. */
int ntm_tee_owner_tick(ntm_tee_owner *,ntm_task *,ntm_task_context *,ntm_task *fallback);
#endif
