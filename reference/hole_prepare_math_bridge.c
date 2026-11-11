/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Expose internal pure arithmetic only to the private original oracle. */
#include "../native-game/hole_prepare.c"
uint32_t ntm_hole_test_scale(uint32_t n){return scaled(n);}
uint32_t ntm_hole_test_direction(const uint8_t *table,unsigned angle){return direction(table,angle);}
