/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-state adapter. */
#define NTM_SHOT_BRIDGE_OPEN
#include "shot_aim_application_bridge.c"
void bridge_open_reset(void){memset(&open_application,0,sizeof open_application);}
int bridge_open_enable(const ntm_shot_open_resources *r)
{return ntm_shot_open_application_enable(&open_application,r);}
void *bridge_open_pointer(unsigned n)
{
 switch(n){case 0:return &open_application.club;case 1:return &open_application.markers;
 case 2:return &open_application.hint;case 3:return &open_application.camera;
 case 4:return open_application.saved_map;case 5:return &open_application.phase;
 case 6:return &open_application.handlers_installed;case 7:return &open_application.input_dispatch;default:return 0;}
}
