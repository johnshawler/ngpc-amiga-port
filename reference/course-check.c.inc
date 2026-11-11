/* SPDX-License-Identifier: GPL-2.0-or-later. Course-handler state/effect oracle.
 * Sound execution and task removal remain external services, not native code. */
#include "course_choice.c"
static ntm_course_choice course_expected;
static unsigned course_active,course_testing,course_effect,course_sound,course_case_effect;
static uint64_t course_results[4];
static uint32_t course_result_address,course_return;
static uint64_t course_calls,course_mismatches,course_unsupported,course_cases,course_sounds,course_finishes;
static unsigned course_ram_pointer(uint32_t address,uint32_t *value)
{
    const uint8_t *p;
    if(address<0x4000 || address>0x7ffc)return 0;
    p=CPUExRAM+address-0x4000;*value=p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return 1;
}
__attribute__((visibility("default")))
uint64_t retro_reference_course_count(unsigned n)
{
    switch(n){case 0:return course_calls;case 1:return course_mismatches;case 2:return course_unsupported;
        case 3:return course_cases;case 4:return course_sounds;case 5:return course_finishes;
        case 6:return course_results[1];case 7:return course_results[2];case 8:return course_results[3];default:return 0;}
}
static void reference_course_before(void)
{
    if(course_testing)return;
    if(pc==0x209536) {
        uint32_t owner;
        if(course_active)course_unsupported++;
        course_active=0;
        if(!course_ram_pointer(gpr[2]+12,&owner) || owner<0x4000 || owner>0x7fee || !course_ram_pointer(gpr[3],&course_return)){course_unsupported++;return;}
        course_result_address=owner+17;course_expected.selection=CPUExRAM[0xfb];
        course_expected.result=CPUExRAM[course_result_address-0x4000];
        course_effect=(unsigned)ntm_course_choice_update(&course_expected,CPUExRAM[0xbf],CPUExRAM[0xc0],CPUExRAM[0x148]);
        course_sound=0;course_active=1;
    }else if(course_active && pc==0x2095a7) {
        if(regL(3)!=0x2501ba || REGA!=2)course_mismatches++;
        course_sound++;course_sounds++;
    }else if(course_active && (pc==course_return || pc==0x20132b)) {
        unsigned finish=pc==0x20132b;
        course_calls++;course_finishes+=finish;
        if(finish && CPUExRAM[course_result_address-0x4000]<4)course_results[CPUExRAM[course_result_address-0x4000]]++;
        if(course_sound>1 || course_effect!=(course_sound?NTM_COURSE_MOVE_SOUND:finish?NTM_COURSE_FINISH:NTM_COURSE_IDLE) || (course_sound && finish))course_mismatches++;
        if(CPUExRAM[0xfb]!=course_expected.selection || CPUExRAM[course_result_address-0x4000]!=course_expected.result)course_mismatches++;
        course_active=0;
    }
}
static unsigned reference_course_case(ntm_course_choice *state,unsigned current,unsigned pressed,unsigned gate)
{
    unsigned steps=0,sounds=0,bad=0,effect;
    CPUExRAM[0xfb]=state->selection;CPUExRAM[0x2111]=state->result;
    CPUExRAM[0xbf]=(uint8_t)current;CPUExRAM[0xc0]=(uint8_t)pressed;CPUExRAM[0x148]=(uint8_t)gate;
    CPUExRAM[0x200c]=0;CPUExRAM[0x200d]=0x61;CPUExRAM[0x200e]=CPUExRAM[0x200f]=0;
    /* Synthetic caller return marker; accepted movement uses eight bytes below. */
    CPUExRAM[0x2be0]=0;CPUExRAM[0x2be1]=0xef;CPUExRAM[0x2be2]=CPUExRAM[0x2be3]=0;
    gpr[2]=0x6000;gpr[3]=0x6be0;pc=0x209536;
    effect=(unsigned)ntm_course_choice_update(state,(uint8_t)current,(uint8_t)pressed,(uint8_t)gate);
    while(pc!=0xef00 && pc!=0x20132b && steps++<256) {
        if(pc==0x2095a7) {
            if(regL(3)!=0x2501ba || REGA!=2)bad++;
            sounds++;pc=0x2095a9; /* Observe request, do not run sound backend. */
        }else TLCS900h_interpret();
    }
    if(pc!=0xef00 && pc!=0x20132b)bad++;
    course_case_effect=sounds?NTM_COURSE_MOVE_SOUND:pc==0x20132b?NTM_COURSE_FINISH:NTM_COURSE_IDLE;
    if(sounds>1 || effect!=course_case_effect)bad++;
    if(state->selection!=CPUExRAM[0xfb] || state->result!=CPUExRAM[0x2111])bad++;
    course_cases++;return bad;
}
__attribute__((visibility("default")))
uint64_t retro_reference_course_selftest(void)
{
    uint8_t ram[16384],controls[16],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;uint16_t save_sr=sr;
    unsigned selection,pressed,option,gate,i;uint64_t failures=0;
    if(course_active || control_active)return 1;
    memcpy(ram,CPUExRAM,sizeof ram);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    course_testing=control_testing=1;
    for(selection=0;selection<256;selection++)for(pressed=0;pressed<256;pressed++)for(option=0;option<2;option++)for(gate=0;gate<2;gate++) {
        ntm_course_choice state={(uint8_t)selection,(uint8_t)(selection^pressed)};
        failures+=reference_course_case(&state,option*64,pressed,gate);
    }
    memset(controls,0,sizeof controls);controls[5]=7;controls[6]=32;
    {
        ntm_course_choice state={0,0},native=state;
        for(i=0;i<4096;i++) {
            uint8_t copy[16];int effect;
            memcpy(copy,controls,16);
            effect=ntm_course_choice_tick(copy,&native,(uint8_t)(i>>2),(uint8_t)(i>>8));
            failures+=reference_control_case(controls,(uint8_t)(i>>2));
            failures+=reference_course_case(&state,controls[0],controls[1],(uint8_t)(i>>8));
            if(memcmp(copy,controls,16) || state.selection!=native.selection || state.result!=native.result || (unsigned)effect!=course_case_effect)failures++;
        }
    }
    for(i=0;i<16384;i++) {
        unsigned allowed=(i>=0xbf && i<0xcf) || i==0xfb || i==0x148 || i==0x2111 ||
            (i>=0x200c && i<0x2010) || (i>=0x2bd8 && i<0x2be4) || i==0x2f82;
        if(!allowed && CPUExRAM[i]!=ram[i])failures++;
    }
    memcpy(CPUExRAM,ram,sizeof ram);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;course_testing=control_testing=0;
    return failures;
}
