/* SPDX-License-Identifier: GPL-2.0-or-later. Synthetic planar throughput. */
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
static uint32_t bench_report[128];
static nr_planar_cache planar_cache;
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
    ((uint32_t *)block)[8]=(uint32_t)bench_report;((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=1;
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    while(beam_line()!=0){}start=ticks();while(beam_line()==0){}while(beam_line()!=0){}finish=ticks();
    r[0]=0x4e474254;r[1]=3;r[2]=709379;r[3]=finish-start;
    /* Scalar/packed/dirty then dirty/packed/scalar; separate conversion passes. */
    for(pass=0;pass<12;pass++) {
        unsigned mode=pass<6?0:pass<9?1:2;
        unsigned variant=pass<3?pass:pass<6?5-pass:pass%3;
        uint32_t checksum=2166136261u;
        ((volatile uint32_t *)block)[10]=10+pass;
        nr_scene_cache_init(&bench_cache);nr_planar_cache_init(&planar_cache);bench_sink=0;
        start=ticks();
        for(frame=0;frame<4;frame++)for(y=0;y<152;y++) {
            if(mode==0)nr_scene_row_cached(&s,y,row,&bench_cache);
            else for(i=0;i<160;i++)row[i]=(uint8_t)((i*7)^((mode==2)?(frame&1):0));
            if(variant==0)nr_store_scene_row(&bitmap,y,row);
            else if(variant==1)nr_store_scene_row_packed(&bitmap,y,row);
            else nr_store_scene_row_dirty(&bitmap,y,row,&planar_cache);
            bench_sink^=row[0]+row[159];
        }
        finish=ticks();
        for(i=0;i<sizeof bench_bitmap;i++)checksum=(checksum^bench_bitmap[i])*16777619u;
        r[4+pass*7]=mode;r[5+pass*7]=variant;r[6+pass*7]=4;r[7+pass*7]=finish-start;
        r[8+pass*7]=checksum;r[9+pass*7]=planar_cache.converted_blocks;r[10+pass*7]=planar_cache.skipped_blocks;
    }
    ((volatile uint32_t *)block)[10]=0x444f4e45;
}
