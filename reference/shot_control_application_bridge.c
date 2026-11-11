/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only adapter. */
#define NTM_SHOT_BRIDGE_CONTROL
#include "shot_open_application_bridge.c"
void bridge_control_reset(void){memset(&control_application,0,sizeof control_application);}
int bridge_control_enable(void){return ntm_shot_control_application_enable(&control_application);}
void *bridge_control_pointer(unsigned n)
{
 switch(n){case 0:return &control_application.camera;case 1:return &control_application.ready;
 case 2:return &control_application.partial_boundary;case 3:return &control_application.ready_update;
 case 4:return app.scroll_x;case 5:return app.scroll_y;default:return 0;}
}
