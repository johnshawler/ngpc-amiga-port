/* SPDX-License-Identifier: GPL-2.0-or-later. Timer/provider original comparisons. */
static uint64_t continuation_cases,random_counts[3];
static uint8_t random_window[256],random_cursor_expected,random_value_expected;
static unsigned random_active,random_ready;
static uint32_t random_return;
static void random_load_window(void)
{
    unsigned i;uint32_t center=loadL(0x260010);
    for(i=0;i<256;i++)random_window[i]=loadB(center-128+i);
    random_ready=1;
}
__attribute__((visibility("default"))) uint64_t retro_reference_random_count(unsigned n)
{return n<3?random_counts[n]:0;}
static void reference_random_before(void)
{
    if(allocation_testing || lifecycle_testing || control_testing || course_testing)return;
    if(pc==0x20197b) {
        if(random_active)random_counts[2]++;
        random_active=0;if(gpr[3]<0x4000 || gpr[3]>0x7ffc){random_counts[2]++;return;}
        if(!random_ready)random_load_window();
        random_cursor_expected=CPUExRAM[14];random_value_expected=(uint8_t)ntm_random_next(&random_cursor_expected,random_window);
        random_return=lifecycle_read(gpr[3]);random_active=1;
    }else if(random_active && pc==random_return) {
        random_counts[0]++;if(CPUExRAM[14]!=random_cursor_expected || REGA!=random_value_expected)random_counts[1]++;
        random_active=0;
    }
}
static unsigned continuation_drive(ntm_task_context *c,ntm_course_parent_state *s,int request,
    unsigned *draws,unsigned *renders,unsigned *sounds)
{
    unsigned bad=0,steps=0;
    while(pc!=0xef00 && steps++<512) {
        if(pc==0x209458) {
            uint8_t cursor=CPUExRAM[14];
            if(request!=NTM_PARENT_RANDOM || regB(0)!=0)bad++;
            bad+=parent_compare(1);(*draws)++;
            request=ntm_course_parent_generate_random(c,parent_nodes,s,&cursor,random_window);
            parent_serialize(c,s);parent_byte(0x400e,cursor);
            TLCS900h_interpret(); /* Execute actual original provider, not a stub. */
        }else if(pc==0x2093d1 || pc==0x209470 || pc==0x2094a6) {
            unsigned arg=pc==0x2093d1?1:pc==0x209470?2:0;
            int wanted=arg==0?NTM_PARENT_SOUND_ZERO:arg==1?NTM_PARENT_SOUND_ONE:NTM_PARENT_SOUND_TWO;
            if(request!=wanted || REGA!=arg || regL(3)!=0x2501ba)bad++;
            bad+=parent_compare(1);(*sounds)++;pc+=2;
            request=ntm_course_parent_resume(c,parent_nodes,s);parent_serialize(c,s);
        }else if(pc==0x2093d9 || pc==0x209464 || pc==0x2094aa) {
            if(request!=NTM_PARENT_RENDER)bad++;
            bad+=parent_compare(1);(*renders)++;pc+=3;
            request=ntm_course_parent_resume(c,parent_nodes,s);parent_serialize(c,s);
        }else if(pc==0x20941d || pc==0x2094f0) {
            if(request!=(pc==0x20941d?NTM_PARENT_FADE:NTM_PARENT_COURSE_SETUP))bad++;
            bad+=parent_compare(1);return bad;
        }else TLCS900h_interpret();
    }
    if(pc!=0xef00 || request!=NTM_PARENT_NONE || gpr[2]!=0x6000 || gpr[3]!=0x6be4)bad++;
    return bad+parent_compare(1);
}
__attribute__((visibility("default"))) uint64_t retro_reference_continuation_count(void){return continuation_cases;}
__attribute__((visibility("default"))) uint64_t retro_reference_continuation_selftest(void)
{
    uint8_t ram[16384],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned kind,timer,i,steps;uint64_t bad=0;ntm_task_context c;ntm_course_parent_state s;
    if(random_active || allocation_active || lifecycle_active || control_active || course_active)return 1;
    memcpy(ram,CPUExRAM,16384);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    allocation_testing=lifecycle_testing=control_testing=course_testing=1;random_load_window();
    for(kind=0;kind<4;kind++)for(timer=0;timer<65536;timer++) {
        int request;unsigned draws=0,renders=0,sounds=0;
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,7,timer&255,0);c.selection=(uint8_t)(timer%3);
        parent_nodes[0].callback=kind==0?parent_cancel:kind==1?parent_accept:kind==2?parent_random_delay:parent_fade;
        parent_nodes[0].delay=(uint16_t)timer;parent_nodes[0].substate=165;s.random_remaining=2;
        s.phase=kind<2?NTM_PARENT_DELAY:kind==2?NTM_PARENT_RANDOM_DELAY:NTM_PARENT_FADE_DELAY;
        parent_serialize(&c,&s);lifecycle_write(0x6be0,0xef00);parent_byte(0x400e,(uint8_t)timer);
        memcpy(CPUExRAM,lifecycle_expected,16384);gpr[2]=0x6000;gpr[3]=0x6be0;pc=parent_callback(parent_nodes[0].callback);
        request=ntm_course_parent_tick(&c,parent_nodes,&s);parent_serialize(&c,&s);
        bad+=continuation_drive(&c,&s,request,&draws,&renders,&sounds);continuation_cases++;
    }
    for(i=0;i<256;i++) {
        uint8_t cursor=(uint8_t)i;int value;
        memcpy(CPUExRAM,ram,16384);CPUExRAM[14]=cursor;
        CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
        memcpy(lifecycle_expected,CPUExRAM,16384);value=ntm_random_next(&cursor,random_window);parent_byte(0x400e,cursor);
        gpr[3]=0x6be0;regL(0)=0x12340000u|(i<<8);pc=0x20197b;
        steps=0;while(pc!=0xef00 && steps++<32)TLCS900h_interpret();
        if(pc!=0xef00 || REGA!=value || regL(0)!=(0x12340000u|(i<<8)|(unsigned)value))bad++;
        bad+=parent_compare(0);continuation_cases++;
    }
    /* A complete30-draw sequence for every initial shared cursor, with actual
     * original provider and controlled sound/render service returns. */
    for(i=0;i<256;i++) {
        unsigned draws=0,renders=0,sounds=0,calls=0;
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,7,i,0);
        parent_nodes[0].callback=parent_wait;parent_nodes[0].result=3;
        parent_serialize(&c,&s);parent_byte(0x400e,(uint8_t)i);lifecycle_write(0x6be0,0xef00);
        memcpy(CPUExRAM,lifecycle_expected,16384);
        do {
            int request;gpr[2]=0x6000;gpr[3]=0x6be0;pc=parent_callback(parent_nodes[0].callback);
            request=calls?ntm_course_parent_tick(&c,parent_nodes,&s):ntm_course_parent_begin(&c,parent_nodes,&s);
            parent_serialize(&c,&s);bad+=continuation_drive(&c,&s,request,&draws,&renders,&sounds);calls++;
        }while(s.phase!=NTM_PARENT_SETUP_PENDING && calls<200);
        if(calls!=91 || draws!=30 || renders!=31 || sounds!=31 || CPUExRAM[14]!=(uint8_t)(i+30))bad++;
        continuation_cases++;
    }
    /* All wrapping remaining-count values at a random timer expiry. */
    for(i=0;i<256;i++) {
        int request;unsigned draws=0,renders=0,sounds=0;
        memcpy(lifecycle_expected,ram,16384);parent_setup(&c,&s,7,i,0);c.selection=(uint8_t)(i%3);
        parent_nodes[0].callback=parent_random_delay;parent_nodes[0].delay=0;
        s.phase=NTM_PARENT_RANDOM_DELAY;s.random_remaining=(uint8_t)i;
        parent_serialize(&c,&s);lifecycle_write(0x6be0,0xef00);memcpy(CPUExRAM,lifecycle_expected,16384);
        gpr[2]=0x6000;gpr[3]=0x6be0;pc=0x209485;
        request=ntm_course_parent_tick(&c,parent_nodes,&s);parent_serialize(&c,&s);
        bad+=continuation_drive(&c,&s,request,&draws,&renders,&sounds);continuation_cases++;
    }
    memcpy(CPUExRAM,ram,16384);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;
    allocation_testing=lifecycle_testing=control_testing=course_testing=0;return bad;
}
