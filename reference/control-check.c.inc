/* SPDX-License-Identifier: GPL-2.0-or-later. Test-only original-game observer.
 * State equivalence only, not a TLCS register/flag/cycle replacement adapter. */
#include <string.h>
#include "input_state.c"
extern uint8_t CPUExRAM[16384];
static uint8_t control_expected[16];
static unsigned control_active,control_testing;
static uint64_t control_calls,control_mismatches,control_unsupported,control_cases;
__attribute__((visibility("default")))
uint64_t retro_reference_control_count(unsigned n)
{
    switch(n){case 0:return control_calls;case 1:return control_mismatches;
        case 2:return control_unsupported;case 3:return control_cases;default:return 0;}
}
static void reference_control_before(void)
{
    unsigned i;
    if(control_testing)return;
    if(pc==0x2002c2) {
        if(control_active)control_unsupported++;
        memcpy(control_expected,CPUExRAM+0xbf,16);
        ntm_input_update(control_expected,CPUExRAM[0x2f82]);
        control_active=1;
    }else if(control_active && pc==0x20042d) {
        control_calls++;
        /* Other processors/peripherals run between TLCS instructions. Their
         * shared RAM updates are not writes performed by this game routine. */
        for(i=0;i<16;i++)if(CPUExRAM[0xbf+i]!=control_expected[i])control_mismatches++;
        control_active=0;
    }else if(control_active && (pc<0x2002c2 || pc>0x20042d)) {
        control_unsupported++;control_active=0;
    }
}
static unsigned reference_control_case(uint8_t state[16],unsigned input)
{
    unsigned steps=0,i,bad=0;
    memcpy(CPUExRAM+0xbf,state,16);CPUExRAM[0x2f82]=(uint8_t)input;
    ntm_input_update(state,(uint8_t)input);pc=0x2002c2;
    while(pc!=0x20042d && steps++<512)TLCS900h_interpret();
    if(pc!=0x20042d)bad++;
    for(i=0;i<16;i++)if(state[i]!=CPUExRAM[0xbf+i])bad++;
    control_cases++;return bad;
}
__attribute__((visibility("default")))
uint64_t retro_reference_control_selftest(void)
{
    uint8_t ram[16384],state[16],save_f=f_dash,save_rfp=statusRFP;
    uint32_t bank[4][4],regs[4],save_pc=pc,save_err=rErr;
    uint16_t save_sr=sr;unsigned a,b,i,k;uint64_t failures=0;
    static const uint8_t masks[4]={0,7,128,255},thresholds[5]={0,32,127,128,255};
    if(control_active)return 1;
    memcpy(ram,CPUExRAM,sizeof ram);memcpy(bank,gprBank,sizeof bank);memcpy(regs,gpr,sizeof regs);
    control_testing=1;
    for(a=0;a<256;a++)for(b=0;b<256;b++) {
        for(i=0;i<16;i++)state[i]=(uint8_t)(a+i*31);
        state[0]=(uint8_t)a;state[5]=7;state[6]=32;
        failures+=reference_control_case(state,b);
    }
    for(a=0;a<256;a++)for(b=0;b<256;b++) {
        for(i=0;i<16;i++)state[i]=(uint8_t)(a*17+b*29+i*31);
        state[0]=255;state[5]=(uint8_t)a;state[6]=(uint8_t)b;
        failures+=reference_control_case(state,255);
    }
    for(k=0;k<256;k++)for(a=0;a<4;a++)for(b=0;b<5;b++) {
        for(i=0;i<16;i++)state[i]=(uint8_t)k;
        state[0]=255;state[5]=masks[a];state[6]=thresholds[b];
        failures+=reference_control_case(state,255);
    }
    /* No routine writes outside its state/input area may be hidden by restore. */
    for(i=0;i<16384;i++)if(!(i>=0xbf && i<0xcf) && i!=0x2f82 && CPUExRAM[i]!=ram[i])failures++;
    memcpy(CPUExRAM,ram,sizeof ram);memcpy(gprBank,bank,sizeof bank);memcpy(gpr,regs,sizeof regs);
    pc=save_pc;sr=save_sr;f_dash=save_f;statusRFP=save_rfp;rErr=save_err;control_testing=0;
    return failures;
}
