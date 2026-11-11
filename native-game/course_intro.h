/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_INTRO_H
#define NTM_COURSE_INTRO_H
#include "task_allocation.h"
#include "course_screen.h"
#include "course_palette.h"
#include "game_services.h"
/* Only fields owned by the normal player-record helper. active_player==0
 * selects bank0; every other byte selects bank1, as in the original. */
typedef struct {
    uint8_t active_player,golfer[2],record_kind[2],active[6];
    uint16_t count[6];
} ntm_course_records;
/* Return0 applied,1 special helper required with NO mutation,-1 invalid index.
 * The special helper is NOT implemented. normal() is also the continuation
 * after a caller has actually completed that separate helper. */
int ntm_course_records_normal(ntm_course_records *records);
int ntm_course_records_prepare(ntm_course_records *records);
enum {NTM_OWNER_NONE=0,NTM_OWNER_SPECIAL_RECORD=1,NTM_OWNER_PRIOR_SELECTION=2,
      NTM_OWNER_ROUND_RECORD=3};
enum {NTM_OWNER_SELECTOR_WAIT=0,NTM_OWNER_SPECIAL_PENDING,
      NTM_OWNER_PRIOR_PENDING,NTM_OWNER_INTRO_WAIT,NTM_OWNER_ROUND_PENDING};
typedef struct {
    ntm_task_callback intro_entry,wait_intro;
    uint8_t phase;
} ntm_course_owner;
/* Call as the real scheduled outer owner, never directly from selector exit.
 * Accepted result0 updates the record and allocates the intro child. Result249
 * waits; all other bytes request the unconverted preceding selection path.
 * Child-wait completion requests round-record without pretending it exists.
 * Requests are stable on repeat; dispatch must not continue past unsupported
 * requests. fallback obeys ntm_task_allocate's reserved-node contract. */
int ntm_course_owner_tick(ntm_task_context *context,ntm_task *owner,
    ntm_course_owner *state,ntm_course_records *records,ntm_task *fallback);
/* ONLY after the special helper really completed. Not a bypass/acknowledgment. */
int ntm_course_owner_special_done(ntm_task_context *context,ntm_task *owner,
    ntm_course_owner *state,ntm_course_records *records,ntm_task *fallback);

typedef struct {
    const uint8_t *descriptor;size_t descriptor_size; /* at least two LE words */
    ntm_screen_asset background; /* resolved entry0 of descriptor's asset group */
    ntm_palette_job palettes[2]; /* resolved commands (course+53)*4 and +12 */
} ntm_course_intro_resources;
typedef struct {
    ntm_course_screen *screen;
    ntm_display_reset_state *display;
    ntm_course_palette *palette;
    uint8_t *maps,*scroll_x,*scroll_y,*flags,*viewport;
    uint16_t *screen_words;
} ntm_course_intro_context;
enum {NTM_INTRO_NEW=0,NTM_INTRO_SCENERY_DELAY,NTM_INTRO_VISIBLE_DELAY,
      NTM_INTRO_TITLE_PENDING};
enum {NTM_INTRO_NONE=0,NTM_INTRO_TITLE=1};
typedef struct {
    ntm_task_callback scenery_delay,visible_delay,title;
    const uint8_t *descriptor_next;
    uint16_t asset_group;
    uint8_t phase;
} ntm_course_intro;
/* Bounded original20771d scenery initializer. Caller selects mapped course0..2
 * resources and keeps borrowed spans alive. Zero-init phase, supply callbacks.
 * Directly clear map0's20x19 region BEFORE reset and queued palette/stamp work.
 * Existing queues, tile memory, sprite memory and unrelated state survive.
 * Queue full silently drops that request; it is not a retry condition.
 * Palette tail, next head, old fill/stamp drains remain scheduler-owned. */
int ntm_course_intro_initialize(ntm_course_intro *state,ntm_task *task,
    ntm_course_intro_context *context,const ntm_course_intro_resources *resources);
/* Signed16-bit modulo delay, including same-call fallthrough to open viewport
 * and first decrement of20 to19. TITLE is a stable unsupported continuation;
 * this is course scenery, not title animation, round-record or playable golf. */
int ntm_course_intro_tick(ntm_course_intro *state,ntm_task *task,
    ntm_course_intro_context *context);
#endif
