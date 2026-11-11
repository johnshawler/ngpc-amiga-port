/* SPDX-License-Identifier: GPL-2.0-or-later. Private original-game oracle. */
#include "task_lifecycle.c"
static uint64_t lifecycle_counts[7];
static unsigned lifecycle_testing,lifecycle_active,lifecycle_outcome;
static uint32_t lifecycle_return,lifecycle_cursor;
static uint8_t lifecycle_expected[16384],lifecycle_mask[16384];
static ntm_task lifecycle_nodes[5];
static uint32_t lifecycle_addresses[5];
static unsigned lifecycle_used,lifecycle_sound_requested;
static void lifecycle_sound(void *unused,uint8_t value){(void)unused;if(value==2)lifecycle_sound_requested++;}
static uint32_t lifecycle_read(uint32_t address)
{
    const uint8_t *p=CPUExRAM+address-0x4000;
    return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void lifecycle_write(uint32_t address,uint32_t value)
{
    unsigned i;for(i=0;i<4;i++){lifecycle_expected[address-0x4000+i]=(uint8_t)(value>>(i*8));lifecycle_mask[address-0x4000+i]=1;}
}
static ntm_task *lifecycle_node(uint32_t address)
{
    unsigned i;if(!address)return 0;
    for(i=0;i<lifecycle_used;i++)if(lifecycle_addresses[i]==address)return &lifecycle_nodes[i];
    if(lifecycle_used==5)return 0;
    lifecycle_addresses[lifecycle_used]=address;return &lifecycle_nodes[lifecycle_used++];
}
static uint32_t lifecycle_address(ntm_task *node)
{
    unsigned i;if(!node)return 0;
    for(i=0;i<lifecycle_used;i++)if(node==&lifecycle_nodes[i])return lifecycle_addresses[i];
    return 0xffffffff;
}
static unsigned lifecycle_prepare(unsigned dispatch)
{
    uint32_t task=gpr[2],previous,next,free_head;
    ntm_task *node;ntm_task_context context;
    if(task<0x4000 || task>0x7fed || gpr[3]<0x4000 || gpr[3]>0x7ffc)return 0;
    previous=lifecycle_read(task+4);next=lifecycle_read(task+8);free_head=lifecycle_read(0x5706);
    if(previous<0x4000 || previous>0x7ff4 || next<0x4000 || next>0x7fef)return 0;
    memset(lifecycle_nodes,0,sizeof lifecycle_nodes);memset(&context,0,sizeof context);lifecycle_used=0;
    node=lifecycle_node(task);node->callback=ntm_task_finish;node->previous=lifecycle_node(previous);node->next=lifecycle_node(next);
    context.free_head=lifecycle_node(free_head);context.current=node;
    node->priority=CPUExRAM[task+16-0x4000];node->flags=CPUExRAM[task+18-0x4000];
    node->next->priority=CPUExRAM[next+16-0x4000];
    memcpy(lifecycle_expected,CPUExRAM,16384);memset(lifecycle_mask,0,16384);
    lifecycle_sound_requested=0;
    if(dispatch==2) {
        uint32_t owner=lifecycle_read(task+12);
        if(owner<0x4000 || owner>0x7fee)return 0;
        node->owner=lifecycle_node(owner);node->owner->result=CPUExRAM[owner+17-0x4000];
        node->callback=ntm_course_task;context.controls=CPUExRAM+0xbf;
        context.selection=CPUExRAM[0xfb];context.gate=CPUExRAM[0x148];context.sound_request=lifecycle_sound;
    }
    lifecycle_outcome=(unsigned)(dispatch?ntm_task_dispatch_one(&context):ntm_task_finish(&context,node));
    if(dispatch)lifecycle_write(0x570a,lifecycle_address(context.published));
    if(lifecycle_outcome==NTM_TASK_DEFERRED)lifecycle_write(task,0x20132b);
    else if(lifecycle_outcome>=2) {
        lifecycle_write(previous+8,lifecycle_address(node->previous->next));
        lifecycle_write(next+4,lifecycle_address(lifecycle_node(next)->previous));
        if(lifecycle_outcome==NTM_TASK_RECYCLED) {
            lifecycle_write(task+8,lifecycle_address(node->next));
            lifecycle_write(0x5706,lifecycle_address(context.free_head));
        }
    }
    if(dispatch==2) {
        uint32_t owner=lifecycle_read(task+12);
        lifecycle_expected[0xfb]=context.selection;lifecycle_mask[0xfb]=1;
        lifecycle_expected[owner+17-0x4000]=node->owner->result;lifecycle_mask[owner+17-0x4000]=1;
    }
    lifecycle_cursor=lifecycle_address(context.current);lifecycle_return=lifecycle_read(gpr[3]);return 1;
}
__attribute__((visibility("default"))) uint64_t retro_reference_lifecycle_count(unsigned n)
{return n<7?lifecycle_counts[n]:0;}
static void reference_lifecycle_before(void)
{
    unsigned i;if(lifecycle_testing)return;
    if(pc==0x20132b) {
        if(lifecycle_active)lifecycle_counts[2]++;
        lifecycle_active=lifecycle_prepare(0);if(!lifecycle_active)lifecycle_counts[2]++;
    }else if(lifecycle_active && pc==lifecycle_return) {
        if(gpr[2]!=lifecycle_cursor)lifecycle_counts[1]++;
        for(i=0;i<16384;i++)if(lifecycle_mask[i] && CPUExRAM[i]!=lifecycle_expected[i])lifecycle_counts[1]++;
        lifecycle_counts[0]++;lifecycle_counts[lifecycle_outcome==1?4:5]++;
        if(lifecycle_outcome==3)lifecycle_counts[6]++;
        lifecycle_active=0;
    }
}
__attribute__((visibility("default"))) uint64_t retro_reference_lifecycle_selftest(void)
{
    uint8_t ram[16384],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned a,b,flags,i,steps;uint64_t bad=0;
    if(lifecycle_active || control_active || course_active)return 1;
    memcpy(ram,CPUExRAM,16384);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    lifecycle_testing=control_testing=course_testing=1;
    for(a=0;a<256;a++)for(b=0;b<256;b++)for(flags=0;flags<2;flags++) {
        memcpy(CPUExRAM,ram,16384);
        /* Three-node linked list plus independent free head. */
        CPUExRAM[0x2004]=0;CPUExRAM[0x2005]=0x61;CPUExRAM[0x2006]=CPUExRAM[0x2007]=0;
        CPUExRAM[0x2008]=0;CPUExRAM[0x2009]=0x62;CPUExRAM[0x200a]=CPUExRAM[0x200b]=0;
        CPUExRAM[0x2010]=(uint8_t)a;CPUExRAM[0x2210]=(uint8_t)b;CPUExRAM[0x2012]=(uint8_t)(0xa0|flags);
        CPUExRAM[0x1706]=0;CPUExRAM[0x1707]=0x63;CPUExRAM[0x1708]=CPUExRAM[0x1709]=0;
        CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
        gpr[2]=0x6000;gpr[3]=0x6be0;pc=0x20132b;
        if(!lifecycle_prepare(0)){bad++;break;}
        /* Original unlink PUSHes the previous cursor then POPs into XIZ. */
        if(lifecycle_outcome!=1)lifecycle_write(gpr[3]-4,0x6100);
        steps=0;while(pc!=0xef00 && steps++<128)TLCS900h_interpret();
        if(pc!=0xef00 || gpr[2]!=lifecycle_cursor)bad++;
        for(i=0;i<16384;i++)if(CPUExRAM[i]!=lifecycle_expected[i])bad++;
        lifecycle_counts[3]++;
    }
    /* Run the actual original dispatcher around a native-converted callback.
     * Its current-task publication and callback-mutated cursor advance matter. */
    for(a=0;a<4096;a++) {
        memcpy(CPUExRAM,ram,16384);
        CPUExRAM[0x2000]=0x2b;CPUExRAM[0x2001]=0x13;CPUExRAM[0x2002]=0x20;CPUExRAM[0x2003]=0;
        CPUExRAM[0x2004]=0;CPUExRAM[0x2005]=0x61;CPUExRAM[0x2006]=CPUExRAM[0x2007]=0;
        CPUExRAM[0x2008]=0;CPUExRAM[0x2009]=0x62;CPUExRAM[0x200a]=CPUExRAM[0x200b]=0;
        CPUExRAM[0x2010]=(uint8_t)a;CPUExRAM[0x2210]=(uint8_t)(a>>4);CPUExRAM[0x2012]=(uint8_t)(a>>8);
        CPUExRAM[0x1706]=CPUExRAM[0x1707]=CPUExRAM[0x1708]=CPUExRAM[0x1709]=255;
        gpr[2]=0x6000;gpr[3]=0x6be4;pc=0x20137c;
        if(!lifecycle_prepare(1)){bad++;break;}
        lifecycle_write(0x6be0,0x201386); /* CALL return on original stack. */
        if(lifecycle_outcome!=1)lifecycle_write(0x6bdc,0x6100);
        steps=0;do {TLCS900h_interpret();steps++;}while(pc!=0x20137c && steps<128);
        if(pc!=0x20137c || gpr[2]!=lifecycle_cursor || gpr[3]!=0x6be4)bad++;
        for(i=0;i<16384;i++)if(CPUExRAM[i]!=lifecycle_expected[i])bad++;
        lifecycle_counts[3]++;
    }
    /* Composed course callback, real original dispatch and lifecycle helper.
     * Sound requests are checked and skipped; audio service is not converted. */
    for(a=0;a<4096;a++) {
        unsigned sounds=0;
        memcpy(CPUExRAM,ram,16384);
        CPUExRAM[0x2000]=0x36;CPUExRAM[0x2001]=0x95;CPUExRAM[0x2002]=0x20;CPUExRAM[0x2003]=0;
        CPUExRAM[0x2004]=0;CPUExRAM[0x2005]=0x61;CPUExRAM[0x2006]=CPUExRAM[0x2007]=0;
        CPUExRAM[0x2008]=0;CPUExRAM[0x2009]=0x62;CPUExRAM[0x200a]=CPUExRAM[0x200b]=0;
        CPUExRAM[0x200c]=0;CPUExRAM[0x200d]=0x64;CPUExRAM[0x200e]=CPUExRAM[0x200f]=0;
        CPUExRAM[0x2010]=0;CPUExRAM[0x2210]=(uint8_t)((a>>10)&1);CPUExRAM[0x2012]=(uint8_t)(a>>11);
        CPUExRAM[0x1706]=CPUExRAM[0x1707]=CPUExRAM[0x1708]=CPUExRAM[0x1709]=255;
        CPUExRAM[0xbf]=(uint8_t)(a>>2);CPUExRAM[0xc0]=(uint8_t)a;
        CPUExRAM[0xfb]=(uint8_t)(a>>8);CPUExRAM[0x148]=(uint8_t)((a>>9)&1);CPUExRAM[0x2411]=165;
        gpr[2]=0x6000;gpr[3]=0x6be4;pc=0x20137c;
        if(!lifecycle_prepare(2)){bad++;break;}
        lifecycle_write(0x6be0,0x201386);
        steps=0;do {
            if(pc==0x2095a7){if(regL(3)!=0x2501ba || REGA!=2)bad++;sounds++;pc=0x2095a9;}
            else TLCS900h_interpret();
            steps++;
        }while(pc!=0x20137c && steps<256);
        if(pc!=0x20137c || gpr[2]!=lifecycle_cursor || gpr[3]!=0x6be4 || sounds!=lifecycle_sound_requested)bad++;
        /* Original PUSH/POP scratch below the CALL return is permitted only
         * in this composed test. All other RAM, including return bytes, exact. */
        for(i=0;i<16384;i++)if(!(i>=0x2bd8 && i<0x2be0) && CPUExRAM[i]!=lifecycle_expected[i])bad++;
        lifecycle_counts[3]++;
    }
    memcpy(CPUExRAM,ram,16384);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;
    lifecycle_testing=control_testing=course_testing=0;return bad;
}
