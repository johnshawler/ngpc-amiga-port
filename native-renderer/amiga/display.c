/* Synthetic emulator-only display fixture. Owns hardware until emulator reset.
 * No OS-return or physical-hardware usability claim. GPL-2.0-or-later. */
#include "../tile_renderer.h"
#ifdef NR_SCENE
#include "scene_fixture.inc"
#endif
#define REG(off) (*(volatile uint16_t *)(0xdff000u+(off)))
#ifdef NR_BENCH
#ifdef NR_PLANAR_BENCH
#include "planar_benchmark.inc"
#elif defined(NR_COMPOSITION_BENCH)
#include "composition_benchmark.inc"
#else
#include "benchmark.inc"
#endif
#endif
static void move(uint16_t **c,unsigned reg,unsigned value){*(*c)++=reg;*(*c)++=value;}
void display_prepare(uint8_t *block)
{
    uint8_t *bitmap=block+64, *tiles=block+64+81920+4096;
    uint16_t *copper=(uint16_t *)(block+64+81920),*c=copper;
    unsigned i,bank,low;
#ifndef NR_SCENE
    uint8_t pens[64];unsigned x,y;
#endif
#ifdef NR_SCENE
    fill_scene(bitmap,tiles);
#else
    nr_bitmap b={bitmap,81920,320,256,40};
    for(i=0;i<8192;i++)tiles[i]=(uint8_t)((i*37u)^(i>>3));
    for(i=0;i<64;i++)pens[i]=(uint8_t)((i/4)*16+(i%4)*5);
    for(y=0;y<24;y++)for(x=0;x<32;x++){
        uint16_t d=(uint16_t)((x+y*32)&511);
        d|=(uint16_t)(((x+y)&15)<<9);
        if(x&1)d|=0x8000;
        if(y&1)d|=0x4000;
        nr_draw_tile(&b,tiles,8192,d,pens,32+x*8,32+y*8);
    }
    #endif
    move(&c,0x100,0x0210); /* COLOR + BPU3 = 8 lowres planes on AGA */
    move(&c,0x102,0);move(&c,0x104,0);move(&c,0x10c,0x0011);
    move(&c,0x1fc,0); /* FMODE: conservative 16-bit fetch */
    move(&c,0x08e,0x2c81);move(&c,0x090,0x2cc1);move(&c,0x1e4,0x2100);
    move(&c,0x092,0x0038);move(&c,0x094,0x00d0);
    move(&c,0x108,0);move(&c,0x10a,0);
    for(i=0;i<8;i++){unsigned p=(unsigned)(bitmap+i*10240);move(&c,0xe0+i*4,p>>16);move(&c,0xe2+i*4,p);}
    for(bank=0;bank<8;bank++)for(low=0;low<2;low++){
        move(&c,0x106,(bank<<13)|(low?0x0200:0));
        for(i=0;i<32;i++){
            unsigned pen=bank*32+i;
            unsigned rgb=((pen&15)<<8)|(((pen>>4)&15)<<4)|((pen^(pen>>4))&15);
            move(&c,0x180+i*2,rgb);
        }
    }
    move(&c,0x106,0);move(&c,0xffff,0xfffe);
    /* Header is entirely in AllocMem(MEMF_CHIP|MEMF_CLEAR) storage. */
    ((uint32_t *)block)[0]=0x4e475044;((uint32_t *)block)[1]=0x49535031; /* NGPDISP1 */
    ((uint32_t *)block)[2]=(unsigned)bitmap;((uint32_t *)block)[3]=(unsigned)copper;
    ((uint32_t *)block)[4]=(unsigned)((uint8_t *)c-(uint8_t *)copper);
    ((uint32_t *)block)[5]=81920;((uint32_t *)block)[6]=(unsigned)block;
    REG(0x09a)=0x7fff;REG(0x09c)=0x7fff;REG(0x096)=0x7fff;
    REG(0x080)=(unsigned)copper>>16;REG(0x082)=(unsigned)copper;
    REG(0x088)=0;REG(0x096)=0x8380;
#ifdef NR_BENCH
    run_benchmark(block,tiles);
#endif
    ((volatile uint32_t *)block)[7]=0x52454144; /* READY */
}
void *memset(void *p,int value,size_t size){unsigned char *s=p;while(size--)*s++=(unsigned char)value;return p;}
