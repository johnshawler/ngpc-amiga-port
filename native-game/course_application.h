/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_APPLICATION_H
#define NTM_COURSE_APPLICATION_H
#include "course_initialization.h"
#include "course_cursor.h"
#include "../native-renderer/scene_renderer.h"
typedef struct {
    const ntm_screen_asset *assets; /*5:two backgrounds,three labels*/
    const ntm_palette_job *palette_jobs; /*3:groups,background,completion*/
    const uint8_t *cursor_descriptor,*fade_in,*fade_out,*random;
    uint8_t *random_cursor;
    const ntm_cursor_assets *cursor; /* NULL only for cursor-free component tests. */
} ntm_course_resources;
typedef struct {
    ntm_course_driver driver;
    ntm_course_initialization initialization;
    ntm_course_screen screen;
    ntm_display_reset_state display;
    ntm_course_palette palette;
    uint8_t controls[16],tiles[8192],maps[4096],sprites[256],sprite_palettes[64];
    uint8_t scroll_x[2],scroll_y[2];
    const ntm_course_resources *resources;
    ntm_course_cursor_engine cursor_engine;
} ntm_course_application;
/* This is a bounded silent/headless course-selector slice, not a full game.
 * Zero-initialize owned nodes before first use. Initialize persistent queue
 * sentinels/control state explicitly; start does not erase old graphics/palette.
 * Supply valid borrowed resources for the lifetime of all queued work.
 * Sound engine, OS input and pacing are
 * NOT implemented here. Optional driver.context.sound_request receives events. */
int ntm_course_application_start(ntm_course_application *app,const ntm_course_resources *resources);
int ntm_course_application_update(ntm_course_application *app,uint8_t buttons);
int ntm_course_application_scene(const ntm_course_application *app,nr_scene *scene);
#endif
