/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "round_prepare.h"
#include "random_byte.h"
int ntm_round_prepare(ntm_round_prepare_state *s,uint8_t *random_cursor,
    const uint8_t random_window[256])
{
    unsigned i,j,bank,attempt,count=0;uint8_t holes[5],cursor;
    if(!s || !random_cursor || !random_window || s->mapped_course>2 ||
        s->course_order_kind!=0)return -1;
    /* The original initializes five words to0x1234 then rejects a drawn pair
     * already present. Valid courses cannot equal that sentinel. Precomputing
     * permits a clean contract error for an otherwise nonterminating window.
     * One complete cursor cycle contains every available candidate. */
    cursor=*random_cursor;
    for(attempt=0;attempt<256 && count<5;attempt++) {
        uint8_t hole=(uint8_t)((unsigned)ntm_random_next(&cursor,random_window)%18u);
        for(j=0;j<count && holes[j]!=hole;j++){}
        if(j==count)holes[count++]=hole;
    }
    if(count!=5)return -1;
    s->round_index=0;s->hole_index=0;
    /* Original20dfe6 runs both records, with4203==255 preserving two fields. */
    for(bank=0;bank<2;bank++) {
        for(i=0x26;i<=0x2a;i++)s->players[bank][i]=0;
        for(i=0x35;i<=0x37;i++)s->players[bank][i]=0;
        if(s->reset_kind!=255) {
            s->players[bank][0x2b]=0;s->players[bank][0x30]=0;
        }
    }
    /* Original20d732 clears the18 stored scores, not the entire player. */
    for(bank=0;bank<2;bank++)for(i=0x10;i<0x22;i++)s->players[bank][i]=255;
    /* Original204aa5 dispatch0: only the first five pairs are overwritten. */
    for(i=0;i<5;i++) {
        s->course_order[2*i]=s->mapped_course;s->course_order[2*i+1]=holes[i];
    }
    *random_cursor=cursor;
    /* Original21290b: any nonzero active-player byte chooses bank1. */
    s->players[s->active_player?1:0][0x34]=s->course_record_flags[s->mapped_course];
    return 0;
}
