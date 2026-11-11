/* SPDX-License-Identifier: GPL-2.0-or-later
 * Bounded native course selector. Emulator takeover until reset, no OS return.
 * Two coherent display buffers; independently paced input/application updates.
 * Original assets are generated locally into ignored build/, never public. */
#include "selector_frontend.h"
#include "selector_memory.h"
#include "selector_assets.inc"
#ifdef NTM_COURSE_INTRO_TARGET
#include "../../native-game/course_intro_application.h"
#include "intro_assets.inc"
static ntm_course_intro_application continuation;
static ntm_course_intro_resources courses[3];
static ntm_course_title_resources titles[3];
static ntm_cursor_assets title_objects[3];
#ifdef NTM_ROUND_RECORD_TARGET
static ntm_round_record_resources round_resources;
static ntm_cursor_assets round_object;
#endif
#define app continuation.selector
#endif
#ifdef SELECTOR_PLANAR_CACHE
#include "selector_planar_cache.h"
#include "selector_planar.inc"
static selector_planar_stats planar_stats;
static unsigned planar_hint;
#endif
#define REG(off) (*(volatile uint16_t *)(0xdff000u+(off)))
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
#define BITMAP_BYTES 81920
#define COPPER_WORDS 1152
#ifndef NTM_COURSE_INTRO_TARGET
static ntm_course_application app;
#endif
static ntm_course_resources resources;
static ntm_screen_asset assets[5];
static ntm_palette_job jobs[3];
static uint8_t random_cursor;
static selector_clock pace;
static nr_scene_cache composition;
static uint32_t composed_rows,reused_rows;
static uint8_t compact[24320];
static struct snapshot {
    nr_scene scene;
    uint8_t tiles[8192],maps[4096],sprites[256],sprite_pal[64],palette[512],scalars[16];
    uint32_t update,selection,invert,cache_index,lookup_ticks;
#ifdef NTM_COURSE_INTRO_TARGET
    uint32_t intro_metadata[16]; /* Immutable metadata for the shown snapshot. */
#ifdef NTM_ROUND_RECORD_TARGET
    uint32_t round_metadata[16];
#endif
#endif
} snapshots[2];
static volatile uint32_t *report;
static uint8_t *bitmaps[2];
static uint16_t *coppers[2];
static uint32_t last_tick;
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
static unsigned beam(void){return ((REG(4)&1)<<8)|(REG(6)>>8);}
static void pump(void)
{
    uint32_t now=ticks(),elapsed=now-last_tick;unsigned due;
    uint8_t sample=selector_joystick(REG(0x00c),*(volatile uint8_t *)0xbfe001u,REG(0x016));
    last_tick=now;if(elapsed>report[16])report[16]=elapsed;
    due=selector_due(&pace,elapsed,sample);
    while(due-- && app.driver.running){
#ifdef NTM_COURSE_INTRO_TARGET
        if(ntm_course_intro_application_update(&continuation,sample)<0)
#else
        if(ntm_course_application_update(&app,sample)<0)
#endif
        {report[2]=0x45525221;for(;;){}}
        if(app.driver.parent.phase==NTM_PARENT_RANDOM_DELAY)report[38]++;
    }
    report[7]=app.driver.updates;report[8]=app.driver.context.selection;
    report[9]=app.driver.running;report[10]=app.driver.nodes[4].result;
    report[11]=sample;report[12]=pace.dropped;report[17]++;
    report[34]=now;report[35]=pace.updates;report[36]=pace.phase;
    report[37]=app.driver.parent.phase;report[39]=random_cursor;
#ifdef NTM_COURSE_INTRO_TARGET
    report[48]=continuation.boundary;report[49]=continuation.owner.phase;
    report[50]=continuation.intro.phase;report[51]=app.driver.parent.mapped_course;
    report[52]=continuation.selector_exit_update;report[53]=continuation.intro_start_update;
    report[54]=continuation.first_visible_update;report[55]=continuation.records.count[0];
    report[56]=continuation.records.active[0];
    report[58]=continuation.title_start_update;report[59]=continuation.tournament_start_update;
    report[60]=continuation.title_complete_update;report[61]=continuation.title.phase;
    report[63]=continuation.tournament_prepare_update;
#ifdef NTM_ROUND_RECORD_TARGET
    report[64]=continuation.round_start_update;report[65]=continuation.round_open_update;
    report[66]=continuation.round_complete_update;report[67]=continuation.round.phase;
    report[70]=continuation.round_drawing_update;
#endif
#endif
}
static void move(uint16_t **c,unsigned reg,unsigned value){*(*c)++=(uint16_t)reg;*(*c)++=(uint16_t)value;}
static unsigned copper(unsigned index)
{
    struct snapshot *s=snapshots+index;
    uint16_t *c=coppers[index];unsigned i,bank,low;
    move(&c,0x100,0x0210);move(&c,0x102,0);move(&c,0x104,0);move(&c,0x10c,0x0011);
    move(&c,0x1fc,0);move(&c,0x08e,0x2c81);move(&c,0x090,0x2cc1);move(&c,0x1e4,0x2100);
    move(&c,0x092,0x0038);move(&c,0x094,0x00d0);move(&c,0x108,0);move(&c,0x10a,0);
    for(i=0;i<8;i++){unsigned p=(unsigned)(bitmaps[index]+i*10240);move(&c,0xe0+i*4,p>>16);move(&c,0xe2+i*4,p);}
    for(bank=0;bank<8;bank++)for(low=0;low<2;low++){
        move(&c,0x106,(bank<<13)|(low?0x0200:0));
        for(i=0;i<32;i++)move(&c,0x180+i*2,selector_color(s->palette,bank*32+i,s->invert));
        pump();
    }
    move(&c,0x106,0);move(&c,0xffff,0xfffe);
    return (unsigned)((uint8_t *)c-(uint8_t *)coppers[index]);
}
static void snapshot(unsigned index)
{
    struct snapshot *s=snapshots+index;nr_scene *n=&s->scene;
    ntm_course_application_scene(&app,n);
    selector_span_copy(s->tiles,n->tiles,8192);selector_span_copy(s->maps,n->maps,4096);
    selector_span_copy(s->sprites,n->sprites,256);selector_span_copy(s->sprite_pal,n->sprite_pal,64);
    selector_span_copy(s->palette,app.palette.palette,512);
    n->tiles=s->tiles;n->maps=s->maps;n->sprites=s->sprites;n->sprite_pal=s->sprite_pal;
    s->update=app.driver.updates;s->selection=app.driver.context.selection;
    /* NGPC control bit7 is the color display's palette inversion flag. */
    s->invert=(app.palette.window_control>>7)&1;
    s->scalars[0]=n->scroll_x[0];s->scalars[1]=n->scroll_x[1];s->scalars[2]=n->scroll_y[0];s->scalars[3]=n->scroll_y[1];
    s->scalars[4]=n->sprite_x;s->scalars[5]=n->sprite_y;s->scalars[6]=n->window_x;s->scalars[7]=n->window_y;
    s->scalars[8]=n->window_w;s->scalars[9]=n->window_h;s->scalars[10]=n->swap;s->scalars[11]=n->background;s->scalars[12]=n->outside;
#ifdef NTM_COURSE_INTRO_TARGET
    {const ntm_course_cursor *c=&continuation.title_cursor;uint32_t *m=s->intro_metadata;
     m[0]=continuation.title.phase;m[1]=continuation.intro.phase;
     m[2]=app.cursor_engine.frame;m[3]=app.cursor_engine.delay;m[4]=continuation.title_x;m[5]=c->y;
     m[6]=c->tile;m[7]=c->flags;m[8]=c->byte14;m[9]=c->byte15;m[10]=c->tag;m[11]=c->width;
     m[12]=c->velocity;m[13]=c->byte30;m[14]=app.display.zero_flags[8];m[15]=app.display.zero_flags[10];}
#ifdef NTM_ROUND_RECORD_TARGET
    {const ntm_course_cursor *c=&continuation.round_cursor;uint32_t *m=s->round_metadata;
     m[0]=continuation.round.phase;m[1]=continuation.round_blink.phase;
     m[2]=app.cursor_engine.frame;m[3]=app.cursor_engine.delay;m[4]=continuation.round_x;m[5]=c->y;
     m[6]=c->tile;m[7]=c->flags;m[8]=c->byte14;m[9]=c->byte15;m[10]=c->tag;m[11]=c->width;
     m[12]=c->velocity;m[13]=c->byte30;m[14]=app.display.zero_flags[8];m[15]=app.display.zero_flags[10];}
#endif
#endif
    report[18]=s->update;report[20]=s->selection;
}
static void present(unsigned index,unsigned clen,uint32_t start)
{
    struct snapshot *s=snapshots+index;uint32_t elapsed;
    /* Finish before the 0x2c display start, allowing Copper setup in blanking. */
    while(beam()>=20)pump();
    REG(0x080)=(unsigned)coppers[index]>>16;REG(0x082)=(unsigned)coppers[index];REG(0x088)=0;
    REG(0x096)=0x8380;
    elapsed=ticks()-start;
    report[4]=(unsigned)bitmaps[index];report[5]=(unsigned)coppers[index];report[6]=clen;
    report[13]++;report[14]=elapsed;if(elapsed>report[31])report[31]=elapsed;
    report[19]=s->update;report[21]=s->selection;
    report[22]=(unsigned)s->tiles;report[23]=(unsigned)s->maps;report[24]=(unsigned)s->sprites;report[25]=(unsigned)s->sprite_pal;
    report[26]=(unsigned)s->palette;report[27]=(unsigned)s->scalars;report[28]=s->invert;
    report[32]=composed_rows;report[33]=reused_rows;
    report[46]=s->cache_index;report[47]=s->lookup_ticks;
#ifdef NTM_COURSE_INTRO_TARGET
    report[57]=(unsigned)s->intro_metadata;report[62]=s->intro_metadata[0];
#ifdef NTM_ROUND_RECORD_TARGET
    report[68]=s->round_metadata[0];report[69]=(unsigned)s->round_metadata;
#endif
#endif
    report[2]=0x52454144;
}
void selector_main(uint8_t *block)
{
    unsigned index=0,y,p,clen;uint32_t start;
    int cached=-1;
    uint8_t dirty[152];nr_bitmap target={compact,sizeof compact,160,152,20};
#ifndef NTM_INTRO_PLANAR_WORDS
    uint8_t row[160];
#endif
    report=(volatile uint32_t *)block;
#ifdef NTM_ROUND_RECORD_TARGET
    report[0]=0x4e524e44;
#elif defined(NTM_COURSE_INTRO_TARGET)
    report[0]=0x4e494e54;
#else
    report[0]=0x4e53454c;
#endif
    report[1]=0x30303031;report[3]=(unsigned)block;
#ifdef NTM_ROUND_RECORD_TARGET
    bitmaps[0]=block+512;
#else
    bitmaps[0]=block+256;
#endif
    bitmaps[1]=bitmaps[0]+BITMAP_BYTES;
    coppers[0]=(uint16_t *)(bitmaps[1]+BITMAP_BYTES);coppers[1]=coppers[0]+COPPER_WORDS;
    selector_initialize(&app);selector_resources(&resources,assets,jobs,&random_cursor);
    nr_scene_cache_init(&composition);
#ifdef NTM_COURSE_INTRO_TARGET
    intro_resources(courses);intro_title_resources(titles,title_objects);
#ifdef NTM_ROUND_RECORD_TARGET
    intro_round_resources(&round_resources,&round_object);
    if(ntm_course_intro_application_start_round_record(&continuation,&resources,courses,titles,title_objects,&round_resources)<0)
#else
    if(ntm_course_intro_application_start_titles(&continuation,&resources,courses,titles,title_objects)<0)
#endif
#else
    if(ntm_course_application_start(&app,&resources)<0)
#endif
    {report[2]=0x45525221;return;}
    REG(0x09a)=0x7fff;REG(0x09c)=0x7fff;REG(0x096)=0x7fff;
    /* CIA B cascaded timers; this takeover owns them until emulator reset. */
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    REG(0x034)=0xff00; /* POTGO: pull up both second joystick button lines. */
    report[29]=SELECTOR_CIA_HZ;report[30]=ticks();last_tick=ticks();
    for(;;){
        pump();start=ticks();snapshot(index);pump();snapshots[index].lookup_ticks=0;
        #ifdef SELECTOR_PLANAR_CACHE
        {uint32_t lookup=ticks(),elapsed;
         cached=selector_planar_find(&snapshots[index].scene,selector_planar_entries,15,planar_hint,&planar_stats,pump);
         elapsed=ticks()-lookup;if(elapsed>report[44])report[44]=elapsed;snapshots[index].lookup_ticks=elapsed;
         report[42]=planar_stats.hits;report[43]=planar_stats.misses;report[45]=sizeof selector_planar_entries;
         if(cached>=0)planar_hint=(unsigned)cached;}
#endif
        snapshots[index].cache_index=(uint32_t)cached;
        if(cached<0)selector_dirty_rows_pump(report[13]?&snapshots[index^1].scene:0,&snapshots[index].scene,dirty,pump);
        pump();
#ifdef SELECTOR_PLANAR_CACHE
        if(cached>=0){
            const uint8_t *source=selector_planar_entries[cached].planar;
            for(y=0;y<152;y+=8){
                unsigned row_y,end=y+8;uint32_t chunk_start=ticks(),elapsed;
                if(end>152)end=152;
                for(row_y=y;row_y<end;row_y++)for(p=0;p<8;p++)selector_span_copy(bitmaps[index]+p*10240+(row_y+52)*40+10,source+p*3040+row_y*20,20);
                reused_rows+=end-y;elapsed=ticks()-chunk_start;
                if(elapsed>report[15])report[15]=elapsed;
                pump();
            }
        }else
#endif
        for(y=0;y<152;y++){
            uint32_t row_start=ticks(),elapsed;
            if(dirty[y]){
#ifdef NTM_INTRO_PLANAR_WORDS
                if(nr_scene_row_planar_words(&snapshots[index].scene,y,&target,&composition)<0){report[2]=0x45525221;return;}
#else
                if(nr_scene_row_cached(&snapshots[index].scene,y,row,&composition)<0 || nr_store_scene_row_packed(&target,y,row)<0){report[2]=0x45525221;return;}
#endif
                for(p=0;p<8;p++)selector_span_copy(bitmaps[index]+p*10240+(y+52)*40+10,compact+p*3040+y*20,20);
                composed_rows++;
            }else{
                for(p=0;p<8;p++){unsigned at=p*10240+(y+52)*40+10;selector_span_copy(bitmaps[index]+at,bitmaps[index^1]+at,20);}
                reused_rows++;
            }
            elapsed=ticks()-row_start;if(elapsed>report[15])report[15]=elapsed;
            pump();
        }
        clen=copper(index);pump();present(index,clen,start);index^=1;
    }
}
void *memset(void *p,int value,size_t size){uint8_t *s=p;while(size--)*s++=(uint8_t)value;return p;}
void *memcpy(void *dst,const void *src,size_t size){uint8_t *d=dst;const uint8_t *s=src;while(size--)*d++=*s++;return dst;}
