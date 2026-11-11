/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
#define NTM_SHOT_BRIDGE_AIM
#include "shot_application_bridge.c"
void bridge_aim_reset(void){memset(&aim_application,0,sizeof aim_application);}
int bridge_aim_enable(const ntm_shot_aim_application_resources *r)
{return ntm_shot_aim_application_enable(&aim_application,r);}
void *bridge_aim_pointer(unsigned n)
{
 switch(n){case 0:return &aim_application.aim;case 1:return &aim_application.world;
 case 2:return aim_application.saved_hud;case 3:return &aim_application.phase;default:return 0;}
}
/* Exact scheduler drawing head only. This test function never dispatches the
 * unconverted209830 callback or advances input, tasks, or update count. */
int bridge_aim_drain_head(void)
{
 ntm_course_driver *driver=&d;
 if(driver->service_pump(driver->service_context)<0 ||
    ntm_course_screen_fill_queue(driver->screen,driver->maps)<0 ||
    ntm_course_screen_stamp_queue(driver->screen,driver->tiles,driver->maps)<0)return -1;
 return driver->drawing_head(driver->service_context);
}
