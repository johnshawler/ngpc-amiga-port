/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "adaptive_renderer.h"
void nr_adaptive_invalidate_all(nr_adaptive_cache *c)
{
    unsigned y;if(!c)return;nr_fine_event_invalidate_all(&c->fine);
    for(y=0;y<152;y++){c->seen[y]=0;c->cooldown[y]=0;}
}
void nr_adaptive_init(nr_adaptive_cache *c)
{
    if(!c)return;
    nr_fine_event_init(&c->fine);nr_adaptive_invalidate_all(c);
    c->fallback_rows=c->cached_rows=0;
}
void nr_adaptive_write(nr_adaptive_cache *c,unsigned kind,unsigned offset)
{
    if(c)nr_fine_event_write(&c->fine,kind,offset);
}
int nr_scene_row_adaptive(const nr_scene *s,unsigned y,uint8_t out[160],nr_adaptive_cache *c)
{
    uint8_t regs[13];unsigned i,changed=0,unknown;
    if(!s || !c || !out || !s->tiles || !s->maps || !s->sprites || !s->sprite_pal || y>=152)return -1;
    regs[0]=s->window_x;regs[1]=s->window_y;regs[2]=s->window_w;regs[3]=s->window_h;
    regs[4]=s->scroll_x[0];regs[5]=s->scroll_y[0];regs[6]=s->scroll_x[1];regs[7]=s->scroll_y[1];
    regs[8]=s->sprite_x;regs[9]=s->sprite_y;regs[10]=s->swap;regs[11]=s->background;regs[12]=s->outside;
    unknown=!c->seen[y];
    if(c->seen[y])for(i=0;i<13;i++)if(regs[i]!=c->previous[y][i]){changed=1;break;}
    for(i=0;i<13;i++)c->previous[y][i]=regs[i];
    c->seen[y]=1;
    if(changed)c->cooldown[y]=2;
    else if(c->cooldown[y])c->cooldown[y]--;
    if(unknown || changed || c->cooldown[y]) {
        c->fine.valid[y]=0;c->fallback_rows++;
        nr_scene_row_cached(s,y,out,&c->fine.sprites);return 1;
    }
    c->cached_rows++;
    return nr_scene_row_fine_event(s,y,out,&c->fine);
}
