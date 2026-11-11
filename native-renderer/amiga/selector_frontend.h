/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SELECTOR_FRONTEND_H
#define SELECTOR_FRONTEND_H
#include "../../native-game/course_application.h"
#define SELECTOR_CIA_HZ 709379u
#define SELECTOR_PHASE_LIMIT (SELECTOR_CIA_HZ*8u)
/* Exact nominal 30.125 source updates/s, not one update per PAL frame.
 * Every call consumes at most two due updates; excess is explicitly dropped. */
typedef struct {uint32_t phase,updates,dropped;uint8_t sample;} selector_clock;
uint8_t selector_joystick(uint16_t joy1dat,uint8_t ciaa_pra,uint16_t potinp);
unsigned selector_due(selector_clock *clock,uint32_t elapsed_ticks,uint8_t sample);
void selector_initialize(ntm_course_application *app);
uint16_t selector_color(const uint8_t palette[512],unsigned pen,unsigned invert);
/* Conservative exact invalidation, no hashes. Marks whole rows affected by
 * changed tile bytes, map descriptors, linked sprites, or scene controls. */
void selector_dirty_rows(const nr_scene *old,const nr_scene *next,uint8_t dirty[152]);

void selector_dirty_rows_pump(const nr_scene *old,const nr_scene *next,uint8_t dirty[152],void (*pump)(void));
#endif
