/* SPDX-License-Identifier: GPL-2.0-or-later. Original allocator comparison. */
#include "task_allocation.c"
static uint64_t allocation_counts[6];
static unsigned allocation_testing,allocation_active,allocation_exhausted;
static uint32_t allocation_return,allocation_result;
static unsigned allocation_prepare(void)
{
    uint32_t parent=gpr[2],head=lifecycle_read(0x5706),next,free_next,callback=regL(0);
    ntm_task_context context;ntm_task *p,*task,*allocated,*fallback;
    if(gpr[3]<0x4000 || gpr[3]>0x7ffc)return 0;
    memset(lifecycle_nodes,0,sizeof lifecycle_nodes);lifecycle_used=0;memset(&context,0,sizeof context);
    memcpy(lifecycle_expected,CPUExRAM,16384);memset(lifecycle_mask,0,16384);
    allocation_exhausted=head==0xffffffff;
    fallback=lifecycle_node(0x4ee6);
    if(allocation_exhausted) {
        allocated=ntm_task_allocate(&context,0,ntm_task_finish,fallback);
    }else {
        if(parent<0x4000 || parent>0x7fef || head<0x4000 || head>0x7fed)return 0;
        next=lifecycle_read(parent+8);free_next=lifecycle_read(head+8);
        if(next<0x4000 || next>0x7ff8)return 0;
        p=lifecycle_node(parent);task=lifecycle_node(head);p->next=lifecycle_node(next);
        task->next=free_next==0xffffffff?0:lifecycle_node(free_next);
        p->priority=CPUExRAM[parent+16-0x4000];task->result=CPUExRAM[head+17-0x4000];
        context.free_head=task;
        allocated=ntm_task_allocate(&context,p,ntm_task_finish,fallback);
        if(allocated!=task || task->callback!=ntm_task_finish || task->result!=CPUExRAM[head+17-0x4000])return 0;
        lifecycle_write(0x5706,context.free_head?lifecycle_address(context.free_head):0xffffffff);
        lifecycle_write(head,callback);lifecycle_write(head+12,lifecycle_address(task->owner));
        lifecycle_expected[head+16-0x4000]=task->priority;lifecycle_mask[head+16-0x4000]=1;
        lifecycle_expected[head+18-0x4000]=task->flags;lifecycle_mask[head+18-0x4000]=1;
        lifecycle_write(parent+8,lifecycle_address(p->next));lifecycle_write(head+4,lifecycle_address(task->previous));
        lifecycle_write(head+8,lifecycle_address(task->next));lifecycle_write(next+4,lifecycle_address(lifecycle_node(next)->previous));
    }
    allocation_result=lifecycle_address(allocated);allocation_return=lifecycle_read(gpr[3]);return 1;
}
__attribute__((visibility("default"))) uint64_t retro_reference_allocation_count(unsigned n)
{return n<6?allocation_counts[n]:0;}
static void reference_allocation_before(void)
{
    unsigned i;if(allocation_testing || lifecycle_testing || control_testing || course_testing)return;
    if(pc==0x2012e7) {
        if(allocation_active || lifecycle_active)allocation_counts[2]++;
        allocation_active=allocation_prepare();if(!allocation_active)allocation_counts[2]++;
    }else if(allocation_active && pc==allocation_return) {
        if(gpr[0]!=allocation_result)allocation_counts[1]++;
        for(i=0;i<16384;i++)if(lifecycle_mask[i] && CPUExRAM[i]!=lifecycle_expected[i])allocation_counts[1]++;
        allocation_counts[0]++;allocation_counts[allocation_exhausted?5:4]++;allocation_active=0;
    }
}
__attribute__((visibility("default"))) uint64_t retro_reference_allocation_selftest(void)
{
    uint8_t ram[16384],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned a,b,alias,i,steps;uint64_t bad=0;
    if(allocation_active || lifecycle_active || control_active || course_active)return 1;
    memcpy(ram,CPUExRAM,16384);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    allocation_testing=lifecycle_testing=control_testing=course_testing=1;
    for(alias=0;alias<2;alias++)for(a=0;a<256;a++)for(b=0;b<256;b++) {
        memcpy(CPUExRAM,ram,16384);
        CPUExRAM[0x2008]=0;CPUExRAM[0x2009]=(uint8_t)(alias?0x60:0x61);CPUExRAM[0x200a]=CPUExRAM[0x200b]=0;
        CPUExRAM[0x2010]=(uint8_t)a;
        CPUExRAM[0x1706]=0;CPUExRAM[0x1707]=0x62;CPUExRAM[0x1708]=CPUExRAM[0x1709]=0;
        CPUExRAM[0x2208]=CPUExRAM[0x2209]=CPUExRAM[0x220a]=CPUExRAM[0x220b]=255;
        CPUExRAM[0x2211]=(uint8_t)b;CPUExRAM[0x2212]=(uint8_t)(a^b);
        CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
        gpr[2]=0x6000;gpr[3]=0x6be0;regL(0)=0x209536;pc=0x2012e7;
        if(!allocation_prepare()){bad++;break;}
        steps=0;while(pc!=0xef00 && steps++<128)TLCS900h_interpret();
        if(pc!=0xef00 || gpr[0]!=allocation_result || gpr[2]!=0x6000)bad++;
        for(i=0;i<16384;i++)if(CPUExRAM[i]!=lifecycle_expected[i])bad++;
        allocation_counts[3]++;
    }
    for(a=0;a<256;a++) {
        memcpy(CPUExRAM,ram,16384);
        CPUExRAM[0x1706]=CPUExRAM[0x1707]=CPUExRAM[0x1708]=CPUExRAM[0x1709]=255;
        CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
        gpr[2]=0x6000+a;gpr[3]=0x6be0;regL(0)=0x209536+a;pc=0x2012e7;
        if(!allocation_prepare()){bad++;break;}
        steps=0;while(pc!=0xef00 && steps++<128)TLCS900h_interpret();
        if(pc!=0xef00 || gpr[0]!=allocation_result || gpr[2]!=0x6000+a)bad++;
        for(i=0;i<16384;i++)if(CPUExRAM[i]!=lifecycle_expected[i])bad++;
        allocation_counts[3]++;
    }
    memcpy(CPUExRAM,ram,16384);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;
    allocation_testing=lifecycle_testing=control_testing=course_testing=0;return bad;
}
