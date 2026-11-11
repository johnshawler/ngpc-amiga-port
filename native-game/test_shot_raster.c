/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "shot_raster.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    ntm_shot_raster s,old;unsigned upper,lower,line,control,run;
    for(upper=0;upper<256;upper++)for(lower=0;lower<256;lower++)for(line=0;line<256;line++) {
        uint8_t raw=73;s.lower_scroll=(uint8_t)lower;s.upper_scroll=(uint8_t)upper;s.callback=1;
        assert(!ntm_shot_raster_irq(&s,(uint8_t)line,&raw));assert(raw==(line<=72?upper:lower));
    }
    for(control=0;control<256;control++)for(run=0;run<256;run++) {
        s=(ntm_shot_raster){17,193,(uint8_t)run,31,12,(uint8_t)control,0,51,(uint8_t)(run^control)};
        if(control&63){old=s;assert(ntm_shot_raster_install(&s)==-1 && ntm_shot_raster_remove(&s)==-1);assert(!memcmp(&s,&old,sizeof s));continue;}
        assert(!ntm_shot_raster_install(&s));assert(s.timer_run==((run&142)|1));
        assert(s.gpu_control==(control|64) && !s.timer_mode && s.timer_divisor==1 && s.callback==1 && !s.counter);
        assert(s.lower_scroll==17 && s.upper_scroll==193 && s.interrupt_shadow==(run^control));
        assert(!ntm_shot_raster_remove(&s));assert(s.timer_run==(run&142) && s.gpu_control==(control&191));
        assert(!s.callback && !s.counter && !s.timer_mode && s.timer_divisor==1 && s.interrupt_shadow==((run^control)&240));
    }
    for(upper=0;upper<256;upper+=17)for(lower=0;lower<256;lower+=17) {
        uint8_t raw=91,latched=53,shown=0;unsigned frame;
        s=(ntm_shot_raster){(uint8_t)lower,(uint8_t)upper,0,0,0,128,0,0,0};
        assert(!ntm_shot_raster_install(&s));assert(raw==91);
        for(frame=0;frame<2;frame++)for(line=0;line<=198;line++) {
            assert(!ntm_shot_raster_step(&s,line,198,&raw,&latched,&shown));
            assert(shown==(!frame && !line?91:line<=72?upper:lower));
            assert(raw==(line<72 || line==198?upper:lower));
        }
        assert(!ntm_shot_raster_remove(&s));raw=11;
        for(line=0;line<=198;line++){assert(!ntm_shot_raster_step(&s,line,198,&raw,&latched,&shown));assert(raw==11 && shown==11);}
    }
    {uint8_t raw=14,latched=29,shown=71;
     s=(ntm_shot_raster){0};old=s;
     assert(ntm_shot_raster_step(&s,199,198,&raw,&latched,&shown)==-1);
     assert(ntm_shot_raster_step(&s,0,150,&raw,&latched,&shown)==-1);
     assert(ntm_shot_raster_step(&s,0,256,&raw,&latched,&shown)==-1);
     assert(ntm_shot_raster_step(&s,0,198,&raw,&raw,&shown)==-1);
     assert(!memcmp(&s,&old,sizeof s) && raw==14 && latched==29 && shown==71);
     s.callback=2;assert(ntm_shot_raster_irq(&s,0,&raw)==-1);assert(raw==14);
     s.callback=1;s.timer_run=1;s.timer_divisor=2;old=s;
     assert(ntm_shot_raster_step(&s,0,198,&raw,&latched,&shown)==-1);assert(!memcmp(&s,&old,sizeof s));
     assert(ntm_shot_raster_install(0)==-1 && ntm_shot_raster_remove(0)==-1);}
    puts("shot raster:16777216 IRQ combinations,65536 register states,152832 line boundaries and atomic rejection pass");return 0;
}
