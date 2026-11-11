/* SPDX-License-Identifier: GPL-2.0-or-later.
 * Synthetic prepared-plane cookie-cut batching experiment. Emulator only.
 * No game renderer, source conversion, clip handling or OS return is claimed.
 */
#define NR_SCENE
#define display_prepare display_fixture_prepare
#include "display.c"
#undef display_prepare

#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
#define LONGREG(off) (*(volatile uint32_t *)(0xdff000u+(off)))
#define PLANE_BYTES 3040u
#define BITMAP_BYTES (8u*PLANE_BYTES)
#define SAMPLE_FRAMES 16u
#define REPORT_WORDS 128u
#define VARIANTS 7u
static uint16_t timer16(unsigned low)
{
    uint8_t hi,again,lo;
    do { hi=CIA(low+0x100);lo=CIA(low);again=CIA(low+0x100); } while(hi!=again);
    return ((uint16_t)hi<<8)|lo;
}
static uint32_t ticks(void)
{
    uint16_t hi,again,lo;
    do { hi=timer16(0x600);lo=timer16(0x400);again=timer16(0x600); } while(hi!=again);
    return ~(((uint32_t)hi<<16)|lo);
}
static unsigned beam_line(void) { return ((REG(4)&1)<<8)|(REG(6)>>8); }
static void wait_frame(void) { while(beam_line()==0){} while(beam_line()!=0){} }
static void wait_blitter(void)
{
    /* First read allows the busy bit to settle on earlier custom chips. */
    (void)REG(2);
    while(REG(2)&0x4000){}
}
static uint32_t digest(const uint8_t *p)
{
    uint32_t v=2166136261u;unsigned n;
    for(n=0;n<BITMAP_BYTES;n++)v=(v^p[n])*16777619u;
    return v;
}
static void prepare_sources(uint8_t *source[2],uint8_t *mask[2],uint8_t *backdrop)
{
    unsigned frame,x,y,plane;
    for(frame=0;frame<2;frame++) {
        memset(source[frame],0,BITMAP_BYTES);memset(mask[frame],0,BITMAP_BYTES);
        for(y=0;y<152;y++)for(x=0;x<160;x++) {
            unsigned tile=x/8+(y/8)*20,u=(tile&1)?7-(x&7):(x&7);
            unsigned v=(tile&2)?7-(y&7):(y&7);
            unsigned code=(u*3+v*5+tile+frame*7)&15;
            unsigned pen=(tile*17+code*13+frame*29)&255;
            unsigned opaque=code!=0 && ((x+y+frame)&7)!=0;
            for(plane=0;plane<8;plane++) {
                unsigned offset=plane*PLANE_BYTES+y*20+x/8;
                unsigned bit=128u>>(x&7);
                if(pen&(1u<<plane))source[frame][offset]|=(uint8_t)bit;
                if(opaque)mask[frame][offset]|=(uint8_t)bit;
            }
        }
    }
    memset(backdrop,0,BITMAP_BYTES);
    for(y=0;y<152;y++)for(x=0;x<160;x++)for(plane=0;plane<8;plane++)
        if(((x*3)^(y*7)^0xa5)&(1u<<plane))
            backdrop[plane*PLANE_BYTES+y*20+x/8]|=(uint8_t)(128u>>(x&7));
}
static void cpu_loop(uint8_t *dst,const uint8_t *src,const uint8_t *mask)
{
    uint32_t *d=(uint32_t *)dst;const uint32_t *s=(const uint32_t *)src,*m=(const uint32_t *)mask;
    unsigned n;
    for(n=0;n<BITMAP_BYTES/4;n++)d[n]=(s[n]&m[n])|(d[n]&~m[n]);
}
static void cpu_unrolled(uint8_t *dst,const uint8_t *src,const uint8_t *mask)
{
    uint32_t *d=(uint32_t *)dst;const uint32_t *s=(const uint32_t *)src,*m=(const uint32_t *)mask;
    unsigned n;
    for(n=0;n<BITMAP_BYTES/4;n+=4) {
        d[n]=(s[n]&m[n])|(d[n]&~m[n]);
        d[n+1]=(s[n+1]&m[n+1])|(d[n+1]&~m[n+1]);
        d[n+2]=(s[n+2]&m[n+2])|(d[n+2]&~m[n+2]);
        d[n+3]=(s[n+3]&m[n+3])|(d[n+3]&~m[n+3]);
    }
}
static void blit(uint8_t *d,const uint8_t *s,const uint8_t *m,unsigned width,unsigned height)
{
    wait_blitter();
    LONGREG(0x050)=(uint32_t)m;LONGREG(0x04c)=(uint32_t)s;
    LONGREG(0x048)=(uint32_t)d;LONGREG(0x054)=(uint32_t)d;
    /* ECS/AGA extended size supports the 1216-line all-plane batch. */
    REG(0x05c)=height;REG(0x05e)=width;
}
static void blitter_draw(uint8_t *d,const uint8_t *s,const uint8_t *m,unsigned variant)
{
    unsigned plane,y,x;
    wait_blitter();
    REG(0x040)=0x0fca;REG(0x042)=0; /* A mask, B pixels, C old dest, D new dest. */
    REG(0x044)=0xffff;REG(0x046)=0xffff;
    REG(0x064)=variant==2?18:0;REG(0x062)=variant==2?18:0;
    REG(0x060)=variant==2?18:0;REG(0x066)=variant==2?18:0;
    if(variant==2)for(plane=0;plane<8;plane++)for(y=0;y<152;y+=8)for(x=0;x<20;x+=2) {
        unsigned off=plane*PLANE_BYTES+y*20+x;
        blit(d+off,s+off,m+off,1,8);
    }
    else if(variant==3)for(plane=0;plane<8;plane++)for(y=0;y<152;y+=8) {
        unsigned off=plane*PLANE_BYTES+y*20;
        blit(d+off,s+off,m+off,10,8);
    }
    else if(variant==4)for(plane=0;plane<8;plane++) {
        unsigned off=plane*PLANE_BYTES;
        blit(d+off,s+off,m+off,10,152);
    }
    else blit(d,s,m,10,1216);
    wait_blitter(); /* Every timed frame includes DMA completion. */
}
void display_prepare(uint8_t *block)
{
    uint8_t *s[2]={block+100000,block+100000+BITMAP_BYTES};
    uint8_t *m[2]={block+100000+2*BITMAP_BYTES,block+100000+3*BITMAP_BYTES};
    uint8_t *dst=block+100000+4*BITMAP_BYTES,*ref=block+100000+5*BITMAP_BYTES;
    uint32_t *r=(uint32_t *)(block+100000+6*BITMAP_BYTES),start,finish;
    unsigned pass,frame,n;
    display_fixture_prepare(block);
    ((volatile uint32_t *)block)[7]=0;
    ((uint32_t *)block)[8]=(uint32_t)r;((uint32_t *)block)[9]=REPORT_WORDS*4;
    ((volatile uint32_t *)block)[10]=1;
    REG(0x096)=0x8040; /* Enable actual Blitter DMA, normal priority. */
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    wait_frame();start=ticks();wait_frame();finish=ticks();
    r[0]=0x424c5452;r[1]=2;r[2]=709379;r[3]=finish-start;
    r[4]=(uint32_t)dst;r[5]=BITMAP_BYTES;r[6]=(uint32_t)ref;r[7]=REG(2);
    r[8]=(uint32_t)s[0];r[9]=(uint32_t)s[1];r[10]=(uint32_t)m[0];r[11]=(uint32_t)m[1];
    start=ticks();prepare_sources(s,m,ref);finish=ticks();r[12]=finish-start;
    r[13]=digest(ref);r[14]=SAMPLE_FRAMES;r[15]=VARIANTS*2;
    for(pass=0;pass<VARIANTS*2;pass++) {
        unsigned variant=pass<VARIANTS?pass:VARIANTS*2-1-pass;
        unsigned mismatches=0,dma;uint32_t *record=r+16+pass*8;
        ((volatile uint32_t *)block)[10]=10+pass;
        /* Fresh deterministic backdrop, outside timed rendering. */
        for(n=0;n<BITMAP_BYTES;n++)dst[n]=ref[n];
        wait_frame();
        if(variant==6)REG(0x096)=0x8400; /* Control: Blitter priority while CPU waits. */
        start=ticks();
        for(frame=0;frame<SAMPLE_FRAMES;frame++) {
            if(variant==0)cpu_loop(dst,s[frame&1],m[frame&1]);
            else if(variant==1)cpu_unrolled(dst,s[frame&1],m[frame&1]);
            else blitter_draw(dst,s[frame&1],m[frame&1],variant);
        }
        finish=ticks();dma=REG(2);
        if(variant==6)REG(0x096)=0x0400;
        /* Independent byte cookie-cut reference outside the timed interval. */
        for(n=0;n<BITMAP_BYTES;n++) {
            uint8_t value=ref[n];
            for(frame=0;frame<SAMPLE_FRAMES;frame++)
                value=(uint8_t)((s[frame&1][n]&m[frame&1][n])|(value&~m[frame&1][n]));
            if(dst[n]!=value)mismatches++;
        }
        record[0]=variant;record[1]=SAMPLE_FRAMES;record[2]=finish-start;record[3]=digest(dst);
        record[4]=mismatches;record[5]=variant<2?0:variant==2?1520:variant==3?152:variant==4?8:1;
        record[6]=dma;record[7]=BITMAP_BYTES;
    }
    ((volatile uint32_t *)block)[10]=0x444f4e45;
    ((volatile uint32_t *)block)[7]=0x52454144;
}
