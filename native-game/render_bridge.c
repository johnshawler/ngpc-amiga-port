/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "render_bridge.h"
int ntm_scene_set_viewport(nr_scene *scene,const uint8_t viewport[4])
{
    if(!scene || !viewport)return -1;
    scene->window_x=viewport[0];scene->window_y=viewport[1];scene->window_w=viewport[2];scene->window_h=viewport[3];return 0;
}
