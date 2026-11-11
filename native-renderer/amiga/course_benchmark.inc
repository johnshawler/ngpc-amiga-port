/* SPDX-License-Identifier: GPL-2.0-or-later. Native active-course pipeline timing. */
#include "../../native-game/input_state.c"
#include "../../native-game/course_choice.c"
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
static uint32_t bench_report[112];
static uint16_t timer16(unsigned low)
{
    uint8_t hi,again,lo;
    do {hi=CIA(low+0x100);lo=CIA(low);again=CIA(low+0x100);}while(hi!=again);
    return ((uint16_t)hi<<8)|lo;
}
static uint32_t ticks(void)
{
    uint16_t high,again,low;
    do {high=timer16(0x600);low=timer16(0x400);again=timer16(0x600);}while(high!=again);
    return ~(((uint32_t)high<<16)|low);
}
static unsigned beam_line(void){return ((REG(4)&1)<<8)|(REG(6)>>8);}
static void run_benchmark(uint8_t *block,uint8_t *tiles)
{
    uint8_t controls[16];unsigned pass,i,k;uint32_t start,finish,*r=bench_report;
    (void)tiles;
    ((uint32_t *)block)[8]=(uint32_t)bench_report;((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=1;
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    while(beam_line()!=0){}start=ticks();while(beam_line()==0){}while(beam_line()!=0){}finish=ticks();
    r[0]=0x4e435342;r[1]=1;r[2]=709379;r[3]=finish-start;
    for(pass=0;pass<8;pass++) {
        unsigned variant=pass<4?pass:7-pass,sounds=0,finishes=0;
        ntm_course_choice choice={1,0xa5};uint32_t checksum=2166136261u;
        for(i=0;i<16;i++)controls[i]=(uint8_t)(i*17);
        controls[5]=7;controls[6]=32;
        ((volatile uint32_t *)block)[10]=10+pass;
        start=ticks();
        if(variant&1)for(i=0;i<16384;i++) {
            int effect=ntm_course_choice_tick(controls,&choice,(uint8_t)(i>>5),(uint8_t)(variant>>1));
            if(effect==NTM_COURSE_MOVE_SOUND)sounds++;
            if(effect==NTM_COURSE_FINISH)finishes++;
        }else for(i=0;i<16384;i++)ntm_input_update(controls,(uint8_t)(i>>5));
        finish=ticks();
        for(i=0;i<16;i++)checksum=(checksum^controls[i])*16777619u;
        checksum=(checksum^choice.selection)*16777619u;checksum=(checksum^choice.result)*16777619u;
        r[4+pass*12]=variant;r[5+pass*12]=16384;r[6+pass*12]=finish-start;r[7+pass*12]=checksum;
        for(k=0;k<4;k++) {
            uint32_t value=0;for(i=0;i<4;i++)value=(value<<8)|controls[k*4+i];
            r[8+pass*12+k]=value;
        }
        r[12+pass*12]=((unsigned)choice.selection<<8)|choice.result;
        r[13+pass*12]=sounds;r[14+pass*12]=finishes;r[15+pass*12]=0;
    }
    ((volatile uint32_t *)block)[10]=0x444f4e45;
}
