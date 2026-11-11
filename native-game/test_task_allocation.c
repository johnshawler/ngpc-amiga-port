/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "task_allocation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int callback(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
int main(void)
{
    unsigned priority,result;unsigned long count=0;
    for(priority=0;priority<256;priority++)for(result=0;result<256;result++) {
        ntm_task parent={0},next={0},a={0},b={0},fallback={0};ntm_task_context c={0};
        parent.next=&next;parent.priority=(uint8_t)priority;parent.result=91;
        next.previous=&parent;a.next=&b;a.result=(uint8_t)result;a.flags=255;
        b.result=(uint8_t)(result^255);c.free_head=&a;c.current=&parent;c.published=&next;
        assert(ntm_task_allocate(&c,&parent,callback,&fallback)==&a);
        assert(c.free_head==&b && c.current==&parent && c.published==&next);
        assert(parent.next==&a && a.previous==&parent && a.next==&next && next.previous==&a);
        assert(a.callback==callback && a.owner==&parent && a.priority==(uint8_t)(priority+1));
        assert(a.result==result && a.flags==0 && parent.result==91);
        assert(ntm_task_allocate(&c,&parent,callback,&fallback)==&b);
        assert(!c.free_head && parent.next==&b && b.next==&a && a.previous==&b);
        assert(b.result==(result^255) && b.priority==(uint8_t)(priority+1));
        assert(ntm_task_allocate(&c,&parent,callback,&fallback)==&fallback);
        assert(!c.free_head && parent.next==&b && fallback.callback==0 && fallback.owner==0);
        count++;
    }
    {
        ntm_task nodes[5]={{0}};ntm_task_context c={0};unsigned i;
        for(i=0;i<4;i++){nodes[i].next=nodes+i+1;nodes[i+1].previous=nodes+i;}
        nodes[0].priority=7;nodes[1].priority=8;nodes[2].priority=9;nodes[3].priority=8;nodes[4].priority=7;
        nodes[2].flags=1;c.current=nodes;
        assert(ntm_task_remove_descendants(&c,nodes)==3);
        assert(nodes[0].next==nodes+4 && nodes[4].previous==nodes && c.current==nodes);
        assert(c.free_head==nodes+3 && nodes[3].next==nodes+1 && !nodes[1].next);
        assert(nodes[2].next==nodes+3 && nodes[2].flags==1);
        assert(ntm_task_remove_descendants(&c,nodes)==0);
    }
    assert(ntm_task_allocate(0,0,0,0)==0);
    printf("PASS native allocation: %lu priority/stale-result sequences\n",count);return 0;
}
