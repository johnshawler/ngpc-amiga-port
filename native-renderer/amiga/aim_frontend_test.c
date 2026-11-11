/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_frontend.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    selector_clock c={0};uint32_t state=0x209830,i;
    for(i=0;i<100000;i++){
        uint32_t elapsed,before,updates,dropped,n;uint64_t total,due;
        state=state*1664525u+1013904223u;elapsed=state;
        before=c.phase;updates=c.updates;dropped=c.dropped;
        total=(uint64_t)before+(uint64_t)elapsed*241u;due=total/SELECTOR_PHASE_LIMIT;
        n=aim_due(&c,elapsed,(uint8_t)i);
        assert(n==(due>8?8:due));assert(c.phase==total%SELECTOR_PHASE_LIMIT);
        assert(c.updates==updates+n && c.dropped==dropped+(uint32_t)(due-n));assert(c.sample==(uint8_t)i);
    }
    c=(selector_clock){0};assert(aim_due(&c,74728,0)==3 && !c.dropped);
    puts("aim clock: 100000 independent64-bit phase/catchup/drop comparisons PASS");return 0;
}
