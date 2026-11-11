/* SPDX-License-Identifier: GPL-2.0-or-later. Native parent state differential test. */
#include "random_byte.c"
#include "viewport.c"
#include "game_services.c"
#include "course_parent.c"
static uint64_t parent_cases;
static int parent_renderer(ntm_task_context *c,ntm_task *t){(void)c;(void)t;
    return 0;}
static int parent_wait(ntm_task_context *c,ntm_task *t){(void)c;(void)t;
    return 0;}
static int parent_cancel(ntm_task_context *c,ntm_task *t){(void)c;(void)t;
    return 0;}
static int parent_accept(ntm_task_context *c,ntm_task *t){(void)c;(void)t;
    return 0;}
static int parent_random_delay(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int parent_random_cycle(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int parent_fade(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int parent_accept_setup(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int parent_cancel_exit(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static int parent_accept_exit(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
static ntm_task parent_nodes[5];
static uint32_t parent_addresses[5]={0x6000,0x6100,0x6200,0x6300,0x4ee6};
static uint32_t parent_pointer(ntm_task *node)
{
    unsigned i;if(!node)return 0xffffffff;
    for(i=0;i<5;i++)if(node==parent_nodes+i)return parent_addresses[i];
    return 0;
}
static uint32_t parent_callback(ntm_task_callback cb)
{
    if(cb==parent_renderer)return 0x2095e3;
    if(cb==parent_wait)return 0x2093b8;
    if(cb==parent_cancel)return 0x2093ed;
    if(cb==parent_accept)return 0x2094cb;
    if(cb==parent_random_delay)return 0x209485;
    if(cb==parent_random_cycle)return 0x209496;
    if(cb==parent_fade)return 0x209416;
    if(cb==parent_accept_setup)return 0x2094dc;
    if(cb==parent_cancel_exit)return 0x20942d;
    if(cb==parent_accept_exit)return 0x209508;
    if(cb==ntm_course_task)return 0x209536;
    return 0;
}
static void parent_byte(uint32_t address,uint8_t value){lifecycle_expected[address-0x4000]=value;}
static void parent_word(uint32_t address,uint16_t value){parent_byte(address,(uint8_t)value);parent_byte(address+1,(uint8_t)(value>>8));}
static void parent_serialize(ntm_task_context *c,ntm_course_parent_state *s)
{
    unsigned i;for(i=0;i<5;i++) {
        ntm_task *n=parent_nodes+i;uint32_t a=parent_addresses[i];
        lifecycle_write(a,parent_callback(n->callback));lifecycle_write(a+4,parent_pointer(n->previous));
        lifecycle_write(a+8,parent_pointer(n->next));lifecycle_write(a+12,parent_pointer(n->owner));
        parent_byte(a+16,n->priority);parent_byte(a+17,n->result);parent_byte(a+18,n->flags);parent_byte(a+19,n->substate);parent_word(a+20,n->delay);
    }
    lifecycle_write(0x5706,parent_pointer(c->free_head));parent_word(0x4545,s->global_word);
    parent_byte(0x40fb,c->selection);parent_byte(0x4204,s->mapped_course);parent_byte(0x454b,s->global_flags);parent_byte(0x4113,s->random_remaining);
}
static void parent_setup(ntm_task_context *c,ntm_course_parent_state *s,unsigned priority,unsigned seed,unsigned pool)
{
    unsigned i;memset(parent_nodes,0,sizeof parent_nodes);memset(c,0,sizeof *c);memset(s,0,sizeof *s);
    s->wait_callback=parent_wait;s->cancel_delay_callback=parent_cancel;s->accept_delay_callback=parent_accept;
    s->random_delay_callback=parent_random_delay;s->random_cycle_callback=parent_random_cycle;
    s->fade_callback=parent_fade;s->setup_callback=parent_accept_setup;
    s->global_word=(uint16_t)(seed*257);s->global_flags=(uint8_t)seed;s->random_remaining=(uint8_t)(seed^0xa5);
    for(i=0;i<5;i++){parent_nodes[i].result=(uint8_t)(seed+i);parent_nodes[i].flags=(uint8_t)(seed^i);parent_nodes[i].delay=(uint16_t)(seed*37+i);}
    parent_nodes[0].priority=parent_nodes[1].priority=(uint8_t)priority;
    parent_nodes[0].next=parent_nodes+1;parent_nodes[0].previous=parent_nodes+1;
    parent_nodes[1].next=parent_nodes;parent_nodes[1].previous=parent_nodes;
    parent_nodes[2].next=pool==2?parent_nodes+3:0;c->free_head=pool?parent_nodes+2:0;c->current=parent_nodes;
}
static unsigned parent_compare(unsigned stack_scratch)
{
    unsigned i,bad=0;for(i=0;i<16384;i++)if(!(stack_scratch && i>=0x2bd0 && i<0x2be0) && CPUExRAM[i]!=lifecycle_expected[i])bad++;
    return bad;
}
__attribute__((visibility("default"))) uint64_t retro_reference_parent_count(void){return parent_cases;}
__attribute__((visibility("default"))) uint64_t retro_reference_parent_selftest(void)
{
    uint8_t ram[16384],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned a,b,pool,steps;uint64_t bad=0;ntm_task_context c;ntm_course_parent_state s;
    if(allocation_active || lifecycle_active || control_active || course_active)return 1;
    memcpy(ram,CPUExRAM,16384);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    allocation_testing=lifecycle_testing=control_testing=course_testing=1;
    for(a=0;a<256;a++)for(pool=0;pool<3;pool++) {
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,a,a^0xa5,pool);parent_serialize(&c,&s);
        lifecycle_write(0x6be0,0xef00);memcpy(CPUExRAM,lifecycle_expected,16384);
        gpr[2]=0x6000;gpr[3]=0x6be0;pc=0x209395;
        if(ntm_course_parent_initialize(&c,parent_nodes,parent_renderer,parent_nodes+4,&s))bad++;
        parent_serialize(&c,&s);
        steps=0;while(pc!=0xef00 && steps++<256)TLCS900h_interpret();
        if(pc!=0xef00 || gpr[2]!=0x6000 || gpr[3]!=0x6be4)bad++;
        bad+=parent_compare(1);parent_cases++;
    }
    /* Every result and parent priority, mixed recycle flags and global bits.
     * Compare at each service boundary before supplying successful stub return. */
    for(a=0;a<256;a++)for(b=0;b<256;b++) {
        int request;unsigned services=0;
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,b,a,2);
        ntm_course_parent_initialize(&c,parent_nodes,parent_renderer,parent_nodes+4,&s);
        parent_nodes[0].result=(uint8_t)a;parent_nodes[2].flags=(uint8_t)(a&1);parent_nodes[3].flags=(uint8_t)(b&1);
        parent_serialize(&c,&s);lifecycle_write(0x6be0,0xef00);memcpy(CPUExRAM,lifecycle_expected,16384);
        gpr[2]=0x6000;gpr[3]=0x6be0;pc=0x2093b8;
        request=ntm_course_parent_begin(&c,parent_nodes,&s);parent_serialize(&c,&s);
        steps=0;
        while(pc!=0xef00 && steps++<512) {
            if(pc==0x2093d1 || pc==0x2094a6) {
                unsigned argument=pc==0x2093d1?1:0;
                if(request!=(argument?NTM_PARENT_SOUND_ONE:NTM_PARENT_SOUND_ZERO) || REGA!=argument || regL(3)!=0x2501ba)bad++;
                bad+=parent_compare(1);services++;pc+=2; /* Preserve original saved-register POPs. */
                request=ntm_course_parent_resume(&c,parent_nodes,&s);parent_serialize(&c,&s);
            }else if(pc==0x2093d9 || pc==0x2094aa) {
                if(request!=NTM_PARENT_RENDER)bad++;
                bad+=parent_compare(1);services++;pc+=3;
                request=ntm_course_parent_resume(&c,parent_nodes,&s);parent_serialize(&c,&s);
            }else if(pc==0x209458) {
                if(request!=NTM_PARENT_RANDOM || regB(0)!=0 || a!=3)bad++;
                bad+=parent_compare(1);services++;break; /* Unconverted random continuation. */
            }else TLCS900h_interpret();
        }
        if(a==3){if(pc!=0x209458 || services!=1)bad++;}
        else {
            if(pc!=0xef00 || request!=NTM_PARENT_NONE || services!=(a?2u:0u) || gpr[2]!=0x6000 || gpr[3]!=0x6be4)bad++;
            bad+=parent_compare(1);
        }
        parent_cases++;
    }
    memcpy(CPUExRAM,ram,16384);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;
    allocation_testing=lifecycle_testing=control_testing=course_testing=0;return bad;
}
