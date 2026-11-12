/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_RESULT_APPLICATION_H
#define NTM_SHOT_RESULT_APPLICATION_H
#include "shot_swing_application.h"
#include "shot_swing_exit.h"
#include "shot_miss.h"
#include "shot_perfect.h"
typedef struct {const ntm_shot_swing_application_resources *swing;const ntm_shot_miss_resources *miss;const ntm_shot_perfect_resources *perfect;} ntm_shot_result_application_resources;
typedef struct {
    ntm_shot_swing_application base;
    ntm_shot_swing_exit exit;
    ntm_shot_miss miss;
    ntm_shot_perfect perfect;
    const ntm_shot_result_application_resources *resources;
    uint8_t enabled,boundary;
} ntm_shot_result_application;
enum {NTM_RESULT_POST_SHOT=8,NTM_RESULT_FLIGHT_ENTRY=9,NTM_RESULT_NEXT_STAGE=10};
/* Opt-in genuine reset/allocation and miss notice. Stops before unconverted
 * post-shot210b74 or newly allocated flight child20e065. */
int ntm_shot_result_application_enable(ntm_shot_result_application *,const ntm_shot_result_application_resources *);
int ntm_shot_result_application_update(ntm_shot_result_application *,uint8_t);
#endif
