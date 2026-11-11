/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_COURSE_DRIVER_H
#define NTM_COURSE_DRIVER_H
#include "course_parent.h"
/* Bounded native course-selector traversal, beginning at original209395.
 * No OS, timer, display pacing or input-device assumptions. The application
 * invokes update at the source logical cadence, not every PAL frame. */
typedef struct {
    ntm_task_context context; /* First member: callbacks recover this owner. */
    ntm_course_parent_state parent;
    ntm_task nodes[5]; /* sentinel,parent,two children,external owner */
    ntm_course_screen *screen;
    ntm_display_reset_state *display;
    const ntm_screen_asset *labels;
    const uint8_t *fade_window,*fade_in_window,*random_window;
    uint8_t *random_cursor,*tiles,*maps;
    int (*service_pump)(void *);
    int (*service_tail)(void *);
    int (*completion_submit)(void *,uint16_t); /* Shared service pool, command84. */
    void *service_context;
    uint32_t updates;
    uint8_t running;
} ntm_course_driver;
/* Caller zero-initializes the driver, supplies all backing states/resources,
 * and initializes context controls/selection/gate/sound plus parent globals and
 * viewport. Screen/background initialization is a separate preceding stage.
 * start establishes owned task nodes and callback identities; no dispatch yet. */
int ntm_course_driver_start(ntm_course_driver *driver);
/* After native background initialization: one reset callback then the original
 * seven-step fade-in, creating selector children during the final fade callback. */
int ntm_course_driver_start_intro(ntm_course_driver *driver,const uint8_t window[256]);
/* input -> service head -> old drawing queues -> ordered tasks -> service tail.
 * New drawing requests remain pending until the NEXT update. Return0 active,
 * 1 parent finished,-1 invalid/error. A callback error leaves the cursor at the
 * failing task; caller must resolve it, not blindly retry a whole update. */
int ntm_course_driver_update(ntm_course_driver *driver,uint8_t buttons);
#endif
