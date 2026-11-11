/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_AIM_H
#define NTM_SHOT_AIM_H
#include "shot_prepare.h"
typedef struct {
    const uint8_t *angles;size_t angles_size; /* 1025 little-endian angle words. */
    const uint8_t *powers[12];size_t power_sizes[12]; /* 13 four-byte club records. */
    const uint8_t *minimums[6];size_t minimum_sizes[6]; /* Signed-byte windows, centre128. */
} ntm_shot_aim_resources;
typedef struct {
    uint16_t angle,initial_angle,angle_delta,origin_x,origin_y;
    uint8_t club,reset_d1,reset_d2,reset_d3,reset_e2,reset_e4,reset_e6,reset_4416;
} ntm_shot_aim;
/* Original20d5ca. Signed16 coordinate comparisons and wrapped differences,
 * integer table ratio; no floating point or approximate replacement. */
int ntm_shot_aim_angle(uint16_t x,uint16_t y,uint16_t target_x,uint16_t target_y,
    const ntm_shot_aim_resources *,uint16_t *angle);
/* Original2050e9, including wrapped16 subtraction. */
uint8_t ntm_shot_aim_at_tee(const uint8_t hole[256]);
/* Original20d8fe including20d9a3 and2050e9. Normal six golfers with modifiers
 * selecting one of twelve resolved power tables. Full signed terrain index. */
int ntm_shot_aim_club(ntm_shot_aim *,const ntm_hole_prepare_state *,
    const ntm_shot_player_state *,const ntm_shot_aim_resources *);
/* Original204e6c. Camera clamp keeps original signed16 comparisons/wrapping. */
int ntm_shot_aim_camera(ntm_shot_aim *,const uint8_t hole[256]);
/*209751..2097f0: clear aim scratch, reject actual green branch, derive angle,
 * club and camera. Stops BEFORE real world-map loader209e28. Returns2 for
 * unsupported terrain3, -1 invalid,0 success. Does not start a shot. */
int ntm_shot_aim_prepare(ntm_shot_aim *,ntm_hole_prepare_state *,
    const ntm_shot_player_state *,const ntm_shot_aim_resources *,uint8_t *player_status,
    uint16_t screen_words[2]);
#endif
