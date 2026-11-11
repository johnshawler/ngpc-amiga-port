/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_POWER_H
#define NTM_SHOT_POWER_H
#include "shot_scene_objects.h"
typedef struct {
    ntm_task_callback meter_entry,power_delay,power_ready;
    ntm_task_callback accuracy_entry,accuracy_delay,accuracy_ready;
    ntm_task_callback cancel_rebuild,cancel_restore,cancel_return,fire_closed;
    uint8_t phase,boundary;
} ntm_shot_power;
typedef struct {
    ntm_shot_scene_object_context *scene;
    uint8_t *overlay; /* Original413b. */
    const uint8_t *saved_meter; /* Four original bytes451f..4522. */
} ntm_shot_power_context;
enum {NTM_SHOT_POWER_WORLD=1,NTM_SHOT_POWER_SWING=2};
/* Exact209a0a: lifecycle2, real HUD restoration, genuine meter allocation and
 * same-call delay4->3. Both child callbacks must identify genuine translated
 * implementations; this parent never dispatches, skips or completes a child. */
int ntm_shot_power_initialize(ntm_shot_power *,ntm_task *,ntm_shot_power_context *);
/* Exact209b71: close viewport and remove actual deeper-priority descendants.
 * Next callback209b9a remains scheduled, rather than run in this same call. */
int ntm_shot_power_cancel(ntm_shot_power *,ntm_task *,ntm_shot_power_context *);
/* Parent209a2e..209aa2 and both cancel continuations. Return2 stops BEFORE
 *209aa2 swing effects or2097f0 world reload; neither is fabricated. Phase8
 * means209bd6 genuinely installed fire_closed with delay3. The owner must
 * hand that callback back to its real first-fire wait implementation.
 * Valid parent tick phases:1..7. A boundary cannot be resumed with this API. */
int ntm_shot_power_tick(ntm_shot_power *,ntm_task *,ntm_shot_power_context *,uint8_t pressed);
#endif
