/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_MATH_H
#define NTM_SHOT_MATH_H
#include <stdint.h>
/* Exact unsigned original20d282 division, including its wrapped remainder and
 * zero-divisor replacement. No host floating point or 64-bit runtime helper. */
uint32_t ntm_shot_divide(uint32_t numerator,uint32_t denominator);
/* Middle32 bits of original20d179 unsigned64-bit product. */
uint32_t ntm_shot_multiply(uint32_t a,uint32_t b);
/* Original20d024 two-refinement distance approximation, not sqrt(). Inputs and
 * result are the original wrapped16.16 bit patterns. */
uint32_t ntm_shot_distance(uint32_t x,uint32_t y,uint32_t target_x,uint32_t target_y);
#endif
