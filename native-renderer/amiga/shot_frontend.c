/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_frontend.h"
#include "selector_memory.h"
int shot_rows_update(shot_rows *s,ntm_shot_raster *r,uint8_t direct)
{
    unsigned f,y;uint8_t raw,latched,shown,upper=0,lower=0;
    if(!s || !r)return -1;
    raw=r->callback?s->raw:direct;latched=s->latched;
    /* Actual live first-tee task writes occur in VBlank before row198.
     * Rows152..197 retain the latch. Row198 primes row0. Coalesce only spans
     * whose repeated step has identical state; do not skip a transition. */
    for(f=0;f<2;f++){
        if(ntm_shot_raster_step(r,198,198,&raw,&latched,&shown)<0 ||
           ntm_shot_raster_step(r,0,198,&raw,&latched,&shown)<0)return -1;
        upper=shown;
        if(ntm_shot_raster_step(r,72,198,&raw,&latched,&shown)<0 ||
           ntm_shot_raster_step(r,73,198,&raw,&latched,&shown)<0)return -1;
        lower=shown;
        if(ntm_shot_raster_step(r,150,198,&raw,&latched,&shown)<0 ||
           ntm_shot_raster_step(r,151,198,&raw,&latched,&shown)<0)return -1;
    }
    for(y=0;y<152;y++)s->plan[y]=y<=72?upper:lower;
    s->raw=raw;s->latched=latched;s->frames+=2;return 0;
}

int shot_rows_uniform(const nr_scene *s,const uint8_t rows[152])
{unsigned y;if(!s || !rows)return 0;for(y=0;y<152;y++)if(rows[y]!=s->scroll_x[0])return 0;return 1;}
void shot_dirty_rows(const nr_scene *old,const uint8_t before[152],const nr_scene *next,
    const uint8_t after[152],uint8_t dirty[152],void (*pump)(void))
{
    unsigned y,vary=0,changed=0;
    selector_dirty_rows_pump(old,next,dirty,pump);
    if(!old)return;
    for(y=0;y<152;y++){
        if(before[y]!=after[y])dirty[y]=1;
        if(before[y]!=old->scroll_x[0] || after[y]!=next->scroll_x[0])vary=1;
    }
    /* Scalar invalidation already covers sprites and all fixed controls. With
     * a varying layer1 plan, a changed map/tile can become visible outside its
     * scalar footprint. Conservatively dirty every row in that case. */
    if(vary){
        for(y=0;y<8192;y+=512){if(!selector_span_equal(old->tiles+y,next->tiles+y,512))changed=1;if(pump)pump();}
        for(y=0;y<2048;y+=512){if(!selector_span_equal(old->maps+y,next->maps+y,512))changed=1;if(pump)pump();}
        if(changed)for(y=0;y<152;y++)dirty[y]=1;
    }
}
