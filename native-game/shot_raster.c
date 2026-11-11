/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_raster.h"
int ntm_shot_raster_install(ntm_shot_raster *s)
{
    if(!s || (s->gpu_control&63))return -1;
    s->callback=1;s->timer_run=(uint8_t)((s->timer_run&0x8eu)|1u);
    s->counter=0;s->timer_mode=0;s->timer_divisor=1;s->gpu_control|=64;return 0;
}
int ntm_shot_raster_remove(ntm_shot_raster *s)
{
    if(!s || (s->gpu_control&63))return -1;
    s->callback=0;s->interrupt_shadow&=0xf0;s->timer_run&=0x8e;s->counter=0;s->gpu_control&=(uint8_t)~64u;return 0;
}
int ntm_shot_raster_irq(const ntm_shot_raster *s,uint8_t line,uint8_t *raw)
{
    if(!s || !raw || s->callback>1)return -1;
    if(s->callback)*raw=line<=72?s->upper_scroll:s->lower_scroll;
    return 0;
}
int ntm_shot_raster_step(ntm_shot_raster *s,unsigned line,unsigned period,
    uint8_t *raw,uint8_t *latched,uint8_t *shown)
{
    unsigned hint;
    if(!s || !raw || !latched || !shown || raw==latched || raw==shown || latched==shown ||
       period<151 || period>255 || line>period || s->callback>1 || (s->gpu_control&63) ||
       ((s->timer_run&1) && ((s->timer_mode&3) || s->timer_divisor!=1 || s->counter)))return -1;
    hint=line<151 || line==period;
    if(hint)*latched=*raw;
    *shown=*latched;
    if(hint && (s->gpu_control&64) && (s->timer_run&1)) {
        /* Divisor1 fires on every eligible HINT and resets its counter. */
        (void)ntm_shot_raster_irq(s,(uint8_t)(line==period?0:line+1),raw);
    }
    return 0;
}
