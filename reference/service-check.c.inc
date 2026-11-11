/* SPDX-License-Identifier: GPL-2.0-or-later. Converted game service comparisons. */
#include "../gfx.h"
static const uint32_t service_zero_addresses[16]={0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8};
static uint64_t service_counts[8];
static unsigned service_active,service_kind,service_expected_carry,service_has_carry;
static uint32_t service_return;
static uint8_t service_viewport[4],fade_window[256],service_payload[9];
static ntm_display_reset_state service_display;
static ntm_service54_state service_queue;
static void service_assets(void)
{
    unsigned i;for(i=0;i<256;i++)fade_window[i]=loadB(0x219867-128+i);
    for(i=0;i<9;i++)service_payload[i]=loadB(0x21a24e + i);
}
static uint16_t service_read_word(uint32_t a){return (uint16_t)(CPUExRAM[a-0x4000]|((uint16_t)CPUExRAM[a+1-0x4000]<<8));}
static void service_serialize_reset(const ntm_display_reset_state *s)
{
    unsigned i;for(i=0;i<16;i++)parent_byte(service_zero_addresses[i],s->zero_flags[i]);
    parent_word(0x4d7b,s->zero_words[0]);parent_word(0x6180,s->zero_words[1]);parent_byte(0x66bc,s->capacity);parent_byte(0x61aa,s->limit);
    for(i=0;i<32;i++){parent_byte(0x453b+i*64,s->object_ids[i]);parent_word(0x453d+i*64,s->object_tags[i]);parent_word(0x4d3b+i*2,s->list[i]);}
    for(i=0;i<256;i++){parent_byte(0x61bb+i,s->pools[0][i]);parent_byte(0x62fb+i,s->pools[1][i]);}
    lifecycle_write(0x5bcb,s->reset_words[0]);lifecycle_write(0x61ac,s->reset_words[1]);
}
static void service_import_queue(ntm_service54_state *s)
{
    unsigned i;s->mode=CPUExRAM[0xfa];s->status=service_read_word(0x66bd);
    for(i=0;i<8;i++) {
        uint32_t a=0x66cf+i*32;ntm_service54_record *r=s->records+i;
        s->slots[i]=service_read_word(0x66bf+i*2);r->code=service_read_word(a);r->command=service_read_word(a+2);
        r->value8=service_read_word(a+8);r->value12=service_read_word(a+12);r->value14=service_read_word(a+14);r->value26=CPUExRAM[a+26-0x4000];
    }
}
static void service_serialize_queue(const ntm_service54_state *s)
{
    unsigned i;parent_word(0x66bd,s->status);
    for(i=0;i<8;i++) {
        uint32_t a=0x66cf+i*32;const ntm_service54_record *r=s->records+i;
        parent_word(0x66bf+i*2,s->slots[i]);parent_word(a,r->code);parent_word(a+2,r->command);
        parent_word(a+8,r->value8);parent_word(a+12,r->value12);parent_word(a+14,r->value14);parent_byte(a+26,r->value26);
    }
}
static void service_serialize_parent(ntm_task_context *c,ntm_course_parent_state *s)
{
    unsigned i;parent_serialize(c,s);parent_byte(0x420b,s->course_index);
    for(i=0;i<3;i++)parent_byte(0x420c+i,s->course_order[i]);
    for(i=0;i<4;i++)service_viewport[i]=s->viewport[i];
}
static unsigned service_prepare(unsigned kind)
{
    unsigned i;int result;ntm_task task={0};uint8_t index,order[3];
    memcpy(lifecycle_expected,CPUExRAM,16384);memset(lifecycle_mask,0,16384);
    for(i=0;i<4;i++)service_viewport[i]=loadB(0x8002+i);
    service_has_carry=0;service_kind=kind;
    if(kind==0) {
        if(gpr[2]<0x4000 || gpr[2]>0x7fec)return 0;
        task.substate=CPUExRAM[gpr[2]+19-0x4000];result=ntm_viewport_fade(&task,service_viewport,fade_window);
        parent_byte(gpr[2]+19,task.substate);service_has_carry=1;service_expected_carry=(unsigned)result;
    }else if(kind==1) {
        result=ntm_course_setup(CPUExRAM[0x204],&index,order);if(result<0)return 0;
        parent_byte(0x420b,index);for(i=0;i<3;i++)parent_byte(0x420c+i,order[i]);
    }else if(kind==2) {
        service_import_queue(&service_queue);result=ntm_service54(&service_queue,service_payload);if(result<0)return 0;
        service_serialize_queue(&service_queue);service_has_carry=1;service_expected_carry=(unsigned)result;
    }else {ntm_display_reset(&service_display);service_serialize_reset(&service_display);}
    /* Live comparison covers only changed owned fields; isolated tests check
     * all RAM. Other processors can update unrelated shared memory live. */
    for(i=0;i<16384;i++)lifecycle_mask[i]=(uint8_t)(lifecycle_expected[i]!=CPUExRAM[i]);
    if(gpr[3]<0x4000 || gpr[3]>0x7ffc)return 0;
    service_return=lifecycle_read(gpr[3]);return 1;
}
static unsigned service_compare(unsigned live)
{
    unsigned i,bad=0;
    for(i=0;i<16384;i++)if((live?lifecycle_mask[i]:!(i>=0x2bd0 && i<0x2be0)) && CPUExRAM[i]!=lifecycle_expected[i])bad++;
    for(i=0;i<4;i++)if(loadB(0x8002+i)!=service_viewport[i])bad++;
    if(service_has_carry && (sr&1)!=service_expected_carry)bad++;
    return bad;
}
__attribute__((visibility("default"))) uint64_t retro_reference_service_count(unsigned n){return n<8?service_counts[n]:0;}
static void reference_service_before(void)
{
    unsigned kind=4;if(allocation_testing || lifecycle_testing || control_testing || course_testing)return;
    if(pc==0x2013a1)kind=0;else if(pc==0x2052ad)kind=1;else if(pc==0x2037a8 && regW(0)==84)kind=2;else if(pc==0x2006cf)kind=3;
    if(kind<4) {
        if(service_active)service_counts[2]++;
        service_assets();service_active=service_prepare(kind);if(!service_active)service_counts[2]++;
    }else if(service_active && pc==service_return) {
        service_counts[0]++;service_counts[4+service_kind]++;service_counts[1]+=service_compare(1);service_active=0;
    }
}
static unsigned service_flow_drive(ntm_task_context *c,ntm_course_parent_state *s,int request)
{
    unsigned bad=0,steps=0;
    while(pc!=0xef00 && steps++<2048) {
        if(pc==0x209458) {
            uint8_t cursor=CPUExRAM[14];if(request!=NTM_PARENT_RANDOM)bad++;
            bad+=service_compare(0);request=ntm_course_parent_generate_random(c,parent_nodes,s,&cursor,random_window);
            service_serialize_parent(c,s);parent_byte(0x400e,cursor);TLCS900h_interpret();
        }else if(pc==0x2093d1 || pc==0x209470 || pc==0x2094a6) {
            unsigned arg=pc==0x2093d1?1:pc==0x209470?2:0;
            if(request!=(arg==0?NTM_PARENT_SOUND_ZERO:arg==1?NTM_PARENT_SOUND_ONE:NTM_PARENT_SOUND_TWO) || REGA!=arg || regL(3)!=0x2501ba)bad++;
            bad+=service_compare(0);pc+=2;request=ntm_course_parent_resume(c,parent_nodes,s);service_serialize_parent(c,s);
        }else if(pc==0x2093d9 || pc==0x209464 || pc==0x2094aa) {
            if(request!=NTM_PARENT_RENDER)bad++;
            bad+=service_compare(0);pc+=3;request=ntm_course_parent_resume(c,parent_nodes,s);service_serialize_parent(c,s);
        }else if(pc==0x20941d) {
            if(request!=NTM_PARENT_FADE)bad++;
            bad+=service_compare(0);request=ntm_course_parent_fade(c,parent_nodes,s,fade_window);
            service_serialize_parent(c,s);TLCS900h_interpret();
        }else if(pc==0x2094f0) {
            if(request!=NTM_PARENT_COURSE_SETUP)bad++;
            bad+=service_compare(0);request=ntm_course_parent_setup(c,parent_nodes,s);service_serialize_parent(c,s);TLCS900h_interpret();
        }else if(pc==0x2094f8) {
            if(request!=NTM_PARENT_SERVICE54 || regW(0)!=84)bad++;
            bad+=service_compare(0);service_import_queue(&service_queue);
            request=ntm_course_parent_service54(c,parent_nodes,s,&service_queue,service_payload);
            service_serialize_queue(&service_queue);service_serialize_parent(c,s);TLCS900h_interpret();
        }else if(pc==0x20942d || pc==0x20951c) {
            if(request!=NTM_PARENT_EXIT)bad++;
            bad+=service_compare(0);request=ntm_course_parent_exit(c,parent_nodes,s,&service_display);
            service_serialize_reset(&service_display);service_serialize_parent(c,s);TLCS900h_interpret();
        }else TLCS900h_interpret();
    }
    if(pc!=0xef00 || request!=0 || gpr[2]!=parent_pointer(c->current) || gpr[3]!=0x6be4)bad++;
    return bad+service_compare(0);
}
__attribute__((visibility("default"))) uint64_t retro_reference_service_selftest(void)
{
    uint8_t ram[16384],save_f=f_dash,save_rfp=statusRFP;ngpgfx_t gpu=*NGPGfx;
    uint32_t bank[4][4],regs[4],addresses[5],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned a,b,k,i,steps;uint64_t bad=0;ntm_task_context c;ntm_course_parent_state s;
    if(service_active || random_active || allocation_active || lifecycle_active || control_active || course_active)return 1;
    memcpy(ram,CPUExRAM,16384);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);memcpy(addresses,parent_addresses,sizeof addresses);
    allocation_testing=lifecycle_testing=control_testing=course_testing=1;service_assets();random_load_window();
    for(k=0;k<4;k++) {
        unsigned count=k==0?65536:k==1?768:k==2?18432:256;
        for(a=0;a<count;a++) {
            for(i=0;i<16384;i++)CPUExRAM[i]=(uint8_t)(i*31+a*17);
            gpr[2]=0x5800;gpr[3]=0x6be0;regW(0)=84;
            CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
            for(i=0;i<4;i++)storeB(0x8002+i,(uint8_t)(a+i*37));
            if(k==0){CPUExRAM[0x1813]=(uint8_t)a;pc=0x2013a1;}
            else if(k==1){CPUExRAM[0x204]=(uint8_t)(a%3);pc=0x2052ad;}
            else if(k==2) {
                unsigned first=(a/256)%9,rotation=a/(256*9);CPUExRAM[0xfa]=(uint8_t)a;pc=0x2037a8;
                for(i=0;i<8;i++){CPUExRAM[0x26bf+i*2]=(uint8_t)(((i+rotation)&7)*4);CPUExRAM[0x26c0+i*2]=(uint8_t)(i>=first?128:32);}
            }else pc=0x2006cf;
            if(!service_prepare(k)){bad++;break;}
            steps=0;while(pc!=0xef00 && steps++<1024)TLCS900h_interpret();
            if(pc!=0xef00)bad++;
            bad+=service_compare(0);service_counts[3]++;
        }
    }
    /* Relocated task fixtures avoid actual reset pools at6200/6300. */
    for(i=0;i<5;i++)parent_addresses[i]=0x5800+i*32;
    for(a=0;a<16;a++)for(b=1;b<4;b++)for(k=0;k<2;k++) {
        unsigned calls=0;int request;
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,7,a,2);
        parent_nodes[0].owner=parent_nodes+4;
        s.cancel_exit_callback=parent_cancel_exit;s.accept_exit_callback=parent_accept_exit;
        s.viewport[0]=3;s.viewport[1]=5;s.viewport[2]=157;s.viewport[3]=147;
        s.course_index=99;s.course_order[0]=s.course_order[1]=s.course_order[2]=99;
        ntm_course_parent_initialize(&c,parent_nodes,parent_renderer,parent_nodes+4,&s);parent_nodes[0].result=(uint8_t)b;
        service_serialize_parent(&c,&s);lifecycle_write(0x6be0,0xef00);parent_byte(0x400e,(uint8_t)a);parent_byte(0x40fa,(uint8_t)(a*17));
        for(i=0;i<8;i++)parent_word(0x66bf+i*2,(uint16_t)((k?0x2000:0x8000)|i*4));
        memcpy(CPUExRAM,lifecycle_expected,16384);for(i=0;i<4;i++)storeB(0x8002+i,s.viewport[i]);service_has_carry=0;
        do {
            gpr[2]=parent_addresses[0];gpr[3]=0x6be0;pc=parent_callback(parent_nodes[0].callback);
            request=calls?ntm_course_parent_tick(&c,parent_nodes,&s):ntm_course_parent_begin(&c,parent_nodes,&s);
            service_serialize_parent(&c,&s);bad+=service_flow_drive(&c,&s,request);calls++;
        }while(s.phase!=NTM_PARENT_DONE && calls<200);
        if(s.phase!=NTM_PARENT_DONE || calls!=(b==1?32u:b==2?13u:92u) || parent_nodes[4].result!=(b==2?252:0))bad++;
        service_counts[3]++;
    }
    memcpy(CPUExRAM,ram,16384);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);memcpy(parent_addresses,addresses,sizeof addresses);*NGPGfx=gpu;
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;
    allocation_testing=lifecycle_testing=control_testing=course_testing=0;return bad;
}
