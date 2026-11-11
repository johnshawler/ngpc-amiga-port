/* SPDX-License-Identifier: GPL-2.0-or-later. Native viewport to planar bridge. */
#include "../../native-game/viewport.c"
#include "../../native-game/render_bridge.c"
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
static uint32_t bench_report[128];

static uint8_t bench_bitmap[24320];
static nr_scene_cache bench_cache;

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
    uint8_t *maps=tiles+8192,*sprites=maps+4096,*pal=sprites+256,row[160],table[256]={0},view[4]={0,2,160,147};
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    nr_bitmap bitmap={bench_bitmap,sizeof bench_bitmap,160,152,20};ntm_task task={0};
    unsigned pass,frame,y,i;uint32_t start,finish,*r=bench_report;
    for(i=0;i<7;i++){table[128+i*2]=(uint8_t)(i*10);table[129+i*2]=(uint8_t)(160-i*20);}
    ((uint32_t *)block)[8]=(uint32_t)bench_report;((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=1;
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    while(beam_line()!=0){}start=ticks();while(beam_line()==0){}while(beam_line()!=0){}finish=ticks();
    r[0]=0x4e564642;r[1]=1;r[2]=709379;r[3]=finish-start;
    for(pass=0;pass<2;pass++) {
        task.substate=0;nr_scene_cache_init(&bench_cache);
        for(frame=0;frame<7;frame++) {
            unsigned busy,errors=0,n=pass*7+frame;uint32_t hash=2166136261u;
            ((volatile uint32_t *)block)[10]=10+n;start=ticks();
            busy=(unsigned)ntm_viewport_fade(&task,view,table);
            if(ntm_scene_set_viewport(&s,view))errors++;
            for(y=0;y<152;y++) {
                if(nr_scene_row_cached(&s,y,row,&bench_cache) || nr_store_scene_row_packed(&bitmap,y,row))errors++;
            }
            finish=ticks();for(i=0;i<sizeof bench_bitmap;i++)hash=(hash^bench_bitmap[i])*16777619u;
            r[4+n*8]=frame;r[5+n*8]=finish-start;r[6+n*8]=hash;r[7+n*8]=view[0];
            r[8+n*8]=view[2];r[9+n*8]=task.substate;r[10+n*8]=busy;r[11+n*8]=errors;
        }
    }
    ((volatile uint32_t *)block)[10]=0x444f4e45;
}
