/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_AIM_SETUP_H
#define NTM_SHOT_AIM_SETUP_H
#include "tee_setup.h"
/* Original209e3a panorama stamp and genuine registered tag5 object producer.
 * Animation service remains separately provided by the caller. */
typedef struct {
    ntm_screen_asset panorama[3][18];
    const uint8_t *metadata;size_t metadata_size;
} ntm_shot_aim_setup_resources;
int ntm_shot_aim_panorama(ntm_tee_setup_context *,const ntm_shot_aim_setup_resources *,
    uint8_t course,uint8_t hole);
#endif
