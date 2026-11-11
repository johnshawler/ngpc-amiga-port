/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "aim_frontend.h"
unsigned aim_due(selector_clock *clock,uint32_t elapsed,uint8_t sample)
{
    uint32_t seconds=elapsed/SELECTOR_CIA_HZ,whole,n;
    elapsed%=SELECTOR_CIA_HZ;clock->sample=sample;
    whole=seconds*30u+seconds/8u;
    clock->phase+=(seconds%8u)*SELECTOR_CIA_HZ+elapsed*241u;
    whole+=clock->phase/SELECTOR_PHASE_LIMIT;clock->phase%=SELECTOR_PHASE_LIMIT;
    n=whole>8u?8u:whole;clock->dropped+=whole-n;clock->updates+=n;return (unsigned)n;
}
