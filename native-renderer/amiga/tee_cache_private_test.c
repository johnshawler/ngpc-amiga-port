/* SPDX-License-Identifier: GPL-2.0-or-later
 * Compile only against ignored locally generated cache includes. */
#include "tee_planar_cache.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define selector_planar_entries baseline_entries
#include "selector_planar.inc"
#undef selector_planar_entries
#include "tee_planar.inc"
static uint8_t output[SELECTOR_PLANAR_BYTES];
int main(void)
{
    unsigned i;
    for(i=0;i<15;i++){
        const selector_planar_entry *a=baseline_entries+i;const tee_planar_entry *b=selector_planar_entries+i;
        nr_scene scene={0};selector_planar_stats stats={0};
        assert(!memcmp(a->maps,b->maps,12688));
        assert(!tee_planar_unpack(b,output,0));assert(!memcmp(a->planar,output,sizeof output));
        scene.tiles=a->tiles;scene.maps=a->maps;scene.sprites=a->sprites;scene.sprite_pal=a->palettes;
        scene.scroll_x[0]=a->controls[0];scene.scroll_x[1]=a->controls[1];scene.scroll_y[0]=a->controls[2];scene.scroll_y[1]=a->controls[3];
        scene.sprite_x=a->controls[4];scene.sprite_y=a->controls[5];scene.window_x=a->controls[6];scene.window_y=a->controls[7];scene.window_w=a->controls[8];scene.window_h=a->controls[9];scene.swap=a->controls[10];scene.background=a->controls[11];scene.outside=a->controls[12];
        assert(tee_planar_find(&scene,selector_planar_entries,15,14-i,&stats,0)==(int)i);
        scene.window_w^=1;assert(tee_planar_find(&scene,selector_planar_entries,15,0,&stats,0)==-1);
    }
    puts("tee private cache: all15 exact keys and364800 decoded picture bytes match original generated entries");return 0;
}
