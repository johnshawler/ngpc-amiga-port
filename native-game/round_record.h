/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_ROUND_RECORD_H
#define NTM_ROUND_RECORD_H
#include "course_title.h"
#include "course_cursor.h"
/* Privately resolved resource spans. No cartridge address is dereferenced by
 * native code. This bounded path supports normal single-player records only. */
typedef struct {
    ntm_palette_job palettes[2];
    ntm_screen_asset font,header,footer,course[3][2],golfer[6][2],labels[2];
    const uint8_t *missing_score;size_t missing_score_size;
    const uint8_t *metadata;size_t metadata_size;
    const ntm_cursor_assets *object;
} ntm_round_record_resources;
typedef struct {
    ntm_course_intro_context *intro;
    ntm_task_context *tasks;ntm_task *fallback;
    ntm_course_cursor *cursor;uint16_t *cursor_x;uint8_t *tiles;
    /* Original41e9,41ec,41ed,414a,417c,417d,4204. Unsupported variants
     * reject before mutation; score255 denotes a fresh/no-result record. */
    uint8_t game_mode,alternate,record_mode,golfer,record_kind,score,course;
    uint8_t input_mode,control,linked_controls[2];
    uint8_t player_markers[2],text_mode;uint16_t text_base;
    int (*prepare)(void *);void *prepare_context;
    void (*sound_request)(void *,uint16_t);void *sound_context;
} ntm_round_record_context;
enum {NTM_ROUND_NEW=0,NTM_ROUND_ALLOCATE_BLINK,NTM_ROUND_OPEN,
      NTM_ROUND_WAIT,NTM_ROUND_DONE};
enum {NTM_ROUND_NONE=0,NTM_ROUND_COMPLETE=1};
typedef struct {
    ntm_task_callback drawing_entry,blink_entry,allocate_blink,open,wait,cleanup;
    uint8_t phase,finish_outcome;
} ntm_round_record;
typedef struct {ntm_task_callback finish;uint16_t asset_group;uint8_t phase;} ntm_round_record_drawing;
typedef struct {ntm_task_callback initial_delay,shown_delay,hidden_delay;uint8_t phase;} ntm_round_record_blink;
/* The preparation callback implements helper204a88, after the65533 sound
 * request and before display mutation. Original208414 is a
 * no-op in this supported record_mode0 path. Sound callbacks record requests,
 * not synthesis equivalence. Existing palette/stamp queues remain intact. */
int ntm_round_record_initialize(ntm_round_record *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
int ntm_round_record_tick(ntm_round_record *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
int ntm_round_record_drawing_initialize(ntm_round_record_drawing *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
int ntm_round_record_drawing_finish(ntm_round_record_drawing *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
int ntm_round_record_blink_initialize(ntm_round_record_blink *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
int ntm_round_record_blink_tick(ntm_round_record_blink *,ntm_task *,ntm_round_record_context *,const ntm_round_record_resources *);
#endif
