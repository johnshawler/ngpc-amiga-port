/* SPDX-License-Identifier: GPL-2.0-or-later. Emulator-only controlled workload.
 * CIA-B timers A/B are reclaimed after OS interrupts are disabled. Timer A
 * counts PAL E-clocks; timer B counts A underflows, giving a 32-bit downcounter. */
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
static uint32_t bench_report[96];
static uint8_t bench_bitmap[24320];
static nr_scene_cache bench_cache;
static volatile uint32_t bench_sink;
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
    uint8_t *maps=tiles+8192,*sprites=maps+4096,*pal=sprites+256,row[160];
    nr_scene s={tiles,maps,sprites,pal,{251,7},{253,19},249,250,3,2,154,147,1,2,5};
    nr_bitmap bitmap={bench_bitmap,sizeof bench_bitmap,160,152,20};
    unsigned pass,frame,y,i;uint32_t start,finish,*r=bench_report;
    ((uint32_t *)block)[8]=(uint32_t)bench_report;
    ((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=1;
    CIA(0xe00)=0;CIA(0xf00)=0; /* stop */
    CIA(0xd00)=0x7f; /* disable CIA-B interrupt masks */
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    ((volatile uint32_t *)block)[10]=2;
    while(beam_line()!=0){}
    ((volatile uint32_t *)block)[10]=3;
    start=ticks();
    while(beam_line()==0){} while(beam_line()!=0){} finish=ticks();
    ((volatile uint32_t *)block)[10]=4;
    r[0]=0x4e474254;r[1]=2;r[2]=709379;r[3]=finish-start;
    /* Stable A/B then B/A controls order; two worst-case live-change passes. */
    for(pass=0;pass<9;pass++) {
        ((volatile uint32_t *)block)[10]=10+pass;
        unsigned cached=(pass==1 || pass==2 || pass==5 || pass==7);
        unsigned mode=pass<4?0:pass<6?1:pass<8?2:3,changing=mode==1;
        unsigned frames=changing?1:4;
        uint32_t checksum=2166136261u;
        pal[0]=0;nr_scene_cache_init(&bench_cache);bench_sink=0;
        for(i=0;i<160;i++)row[i]=(uint8_t)(i*7);
        start=ticks();
        for(frame=0;frame<frames;frame++)for(y=0;y<152;y++) {
            if(changing)pal[0]=(uint8_t)(y&1);
            if(mode!=3) {
                if(cached)nr_scene_row_cached(&s,y,row,&bench_cache);
                else nr_scene_row(&s,y,row);
            }
            if(mode!=2)nr_store_scene_row(&bitmap,y,row);
            bench_sink^=row[0]+row[159];
        }
        finish=ticks();
        /* Digest outside timed region, so equivalent outputs are checked. */
        if(mode==2)for(i=0;i<160;i++)checksum=(checksum^row[i])*16777619u;
        else for(i=0;i<sizeof bench_bitmap;i++)checksum=(checksum^bench_bitmap[i])*16777619u;
        r[4+pass*7]=mode;r[5+pass*7]=cached;r[6+pass*7]=frames;
        r[7+pass*7]=finish-start;r[8+pass*7]=checksum;
        r[9+pass*7]=bench_cache.rebuilds;r[10+pass*7]=bench_cache.hits;
    }
    pal[0]=0;
    ((uint32_t *)block)[8]=(uint32_t)bench_report;
    ((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=0x444f4e45;
}
