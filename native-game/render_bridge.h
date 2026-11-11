/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_RENDER_BRIDGE_H
#define NTM_RENDER_BRIDGE_H
#include "../native-renderer/scene_renderer.h"
/* Apply converted game viewport to the native compositor's row-latched scene.
 * Caller owns scheduling/latching, palette and valid backing graphics arrays. */
int ntm_scene_set_viewport(nr_scene *scene,const uint8_t viewport[4]);
#endif
