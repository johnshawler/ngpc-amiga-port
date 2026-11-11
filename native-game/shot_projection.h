/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_PROJECTION_H
#define NTM_SHOT_PROJECTION_H
#include <stdint.h>
typedef struct {
    uint16_t x,y;
    uint32_t depth,lateral_scratch,depth_scratch;
} ntm_shot_projection;
/* Exact20f98a perspective projection, including20d541 signed division.
 * All inputs are wrapped16.16 bit patterns. No floating point,64-bit runtime
 * arithmetic or host-dependent signed shifts. Scratch maps original5bcb/5bcf.
 * x/y are projected signed16 screen coordinates; depth is the original XDE.
 * This arithmetic helper does not draw, sample terrain, or start a swing. */
int ntm_shot_project(uint32_t x,uint32_t y,uint32_t origin_x,uint32_t origin_y,
    uint32_t direction_x,uint32_t direction_y,ntm_shot_projection *);
#endif
