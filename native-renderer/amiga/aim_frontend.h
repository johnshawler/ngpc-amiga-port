/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef AIM_FRONTEND_H
#define AIM_FRONTEND_H
#include "shot_frontend.h"
/* Same241/8-Hz source timebase. Up to8 due updates catch up actual elapsed
 * time; larger stalls explicitly count dropped updates. This does not lengthen
 * any game delay or reconstruct inputs missed between hardware samples. */
unsigned aim_due(selector_clock *,uint32_t elapsed,uint8_t sample);
#endif
