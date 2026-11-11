/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_VIEWPORT_H
#define NTM_VIEWPORT_H
#include "task_lifecycle.h"
/* Private 256-byte signed-index asset window centered 128 bytes before original
 * table pointer. Updates X/width only, preserves Y/height. Returns busy 0/1 or -1.
 * All byte indices are supported, including signed-index and increment wrap. */
int ntm_viewport_fade(ntm_task *task,uint8_t viewport[4],const uint8_t window[256]);
/* Valid mapped-course domain is 0..2. Resets index, writes cyclic course order. */
int ntm_course_setup(uint8_t mapped,uint8_t *index,uint8_t order[3]);
#endif
