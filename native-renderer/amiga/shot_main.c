#define NTM_COURSE_INTRO_TARGET
#define NTM_ROUND_RECORD_TARGET
/* SPDX-License-Identifier: GPL-2.0-or-later
 * Bounded native course selector. Emulator takeover until reset, no OS return.
 * Two coherent display buffers; independently paced input/application updates.
 * Original assets are generated locally into ignored build/, never public. */
#include "selector_frontend.h"
#include "selector_memory.h"
#include "selector_assets.inc"
#ifdef NTM_COURSE_INTRO_TARGET
#include "../../native-game/shot_application.h"
#include "shot_frontend.h"
#include "shot_assets.inc"
#include "intro_assets.inc"
static ntm_shot_application shot;
#define tee shot.base
static ntm_shot_resources shot_resource;
static ntm_shot_prepare_resources preparation_resources;
static ntm_terrain_resources terrain_resources;
static ntm_shot_banner_resources banner_resources;
static ntm_tee_object_assets banner_objects;
static ntm_shot_palette_job exit_palettes[2];
static shot_rows raster_rows;
static uint8_t sampled_inputs[4096];
#define continuation tee.base
static ntm_tee_setup_resources overview_resources;
static ntm_tee_object_assets overview_objects;
static ntm_hole_prepare_resources hole_resources;
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
#include "tee_planar_cache.h"
#include "tee_planar.inc"
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
    uint32_t tee_metadata[128];
    uint32_t shot_metadata[64];
    uint8_t row_scroll[152];
#endif
#endif
} snapshots[2];
static volatile uint32_t *report;
static uint8_t *bitmaps[2];
static uint16_t *coppers[2];
static uint32_t last_tick;
static uint32_t uploaded_metadata[64];
static void tee_state_metadata(uint32_t *m)
{
    unsigned i;
    m[0]=tee.overview.phase;m[1]=tee.started;m[2]=tee.boundary;m[3]=app.display.zero_flags[8];
    m[4]=tee.context.course;m[5]=tee.context.round_index;m[6]=tee.context.hole_index;
    m[7]=tee.context.par;m[8]=tee.context.distance;m[9]=app.display.zero_flags[10];
    m[10]=tee.engine.uploaded_bank;m[11]=random_cursor;
    for(i=0;i<4;i++){const ntm_course_cursor *o=tee.objects+i;unsigned at=12+13*i;
      m[at]=o->tag;m[at+1]=tee.x[i];m[at+2]=o->y;m[at+3]=o->tile;
      m[at+4]=o->flags;m[at+5]=o->byte14;m[at+6]=o->byte15;m[at+7]=o->velocity;
      m[at+8]=tee.engine.frame[i];m[at+9]=tee.engine.delay[i];m[at+10]=o->second;
      m[at+11]=o->width;m[at+12]=o->id;}
}
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
        if(app.driver.updates>=sizeof sampled_inputs){report[2]=0x45525221;for(;;){}}
        sampled_inputs[app.driver.updates]=sample;
        if(tee.started && (app.initialization.flags&80)==80)tee_state_metadata(uploaded_metadata);
#ifdef NTM_COURSE_INTRO_TARGET
        if(ntm_shot_application_update(&shot,sample)<0 || shot_rows_update(&raster_rows,&shot.raster,app.scroll_x[0])<0)
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
    report[72]=tee.start_update;report[73]=tee.overview_update;report[74]=tee.open_update;
    report[75]=tee.hold_update;report[76]=tee.complete_update;report[77]=tee.boundary;
    report[78]=tee.overview.phase;report[79]=tee.owner.phase;
    report[82]=tee.context.course;report[83]=tee.context.round_index;report[84]=tee.context.hole_index;
    report[85]=tee.context.par;report[86]=tee.context.distance;
    report[91]=shot.start_update;report[92]=shot.banner_update;report[93]=shot.complete_update;
    report[94]=shot.boundary;report[95]=shot.banner.phase;report[98]=shot.raster.callback;
    report[99]=shot.raster.upper_scroll;report[101]=(unsigned)sampled_inputs;report[102]=app.driver.updates;
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
    tee_state_metadata(s->tee_metadata);
    selector_span_copy(s->row_scroll,raster_rows.plan,152);
    {uint32_t *m=s->shot_metadata;unsigned j;const ntm_course_cursor *o=tee.objects+11;
     m[0]=shot.started;m[1]=shot.boundary;m[2]=shot.scene.phase;m[3]=shot.banner.phase;
     m[4]=shot.raster.lower_scroll;m[5]=shot.raster.upper_scroll;m[6]=shot.raster.callback;
     m[7]=raster_rows.raw;m[8]=raster_rows.latched;m[9]=raster_rows.frames;
     m[10]=shot.animation.phase;m[11]=shot.exit.phase;m[12]=shot.banner.blink_delay;m[13]=shot.banner.blink_state;
     m[14]=o->tag;m[15]=tee.x[11];m[16]=o->y;m[17]=o->tile;m[18]=o->velocity;
     m[19]=tee.engine.frame[11];m[20]=tee.engine.delay[11];m[21]=o->flags;m[22]=o->byte30;
     for(j=0;j<8;j++){m[24+3*j]=shot.palette.jobs[j].kind;m[25+3*j]=shot.palette.level[j];m[26+3*j]=shot.palette.fraction[j];}}
    selector_span_copy((uint8_t *)(s->tee_metadata+64),(const uint8_t *)uploaded_metadata,sizeof uploaded_metadata);
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
    report[80]=s->tee_metadata[0];report[81]=(unsigned)s->tee_metadata;
    report[96]=s->shot_metadata[3];report[97]=(unsigned)s->shot_metadata;report[100]=(unsigned)s->row_scroll;
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
    report[0]=0x4e534854;
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
    tee_resources(&overview_resources,&overview_objects,&hole_resources);
    shot.raster.timer_run=136;shot.raster.gpu_control=128;
    shot_resources(&shot_resource,&preparation_resources,&terrain_resources,&banner_resources,&banner_objects,exit_palettes);
    if(ntm_shot_application_start(&shot,&resources,courses,titles,title_objects,&round_resources,&overview_resources,&hole_resources,&shot_resource)<0)
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
         cached=shot_rows_uniform(&snapshots[index].scene,snapshots[index].row_scroll)?tee_planar_find(&snapshots[index].scene,selector_planar_entries,15,planar_hint,&planar_stats,pump):-1;
         elapsed=ticks()-lookup;if(elapsed>report[44])report[44]=elapsed;snapshots[index].lookup_ticks=elapsed;
         report[42]=planar_stats.hits;report[43]=planar_stats.misses;report[45]=sizeof selector_planar_entries+TEE_PACKED_PICTURE_BYTES;
         if(cached>=0)planar_hint=(unsigned)cached;}
#endif
        snapshots[index].cache_index=(uint32_t)cached;
        if(cached<0)shot_dirty_rows(report[13]?&snapshots[index^1].scene:0,snapshots[index^1].row_scroll,&snapshots[index].scene,snapshots[index].row_scroll,dirty,pump);
        pump();
#ifdef SELECTOR_PLANAR_CACHE
        if(cached>=0){
            const uint8_t *source=compact;
            {uint32_t decode=ticks(),elapsed;
             if(tee_planar_unpack(selector_planar_entries+cached,compact,pump)<0){report[2]=0x45525221;return;}
             elapsed=ticks()-decode;report[89]=elapsed;if(elapsed>report[88])report[88]=elapsed;report[90]++;}
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
                nr_scene current=snapshots[index].scene;current.scroll_x[0]=snapshots[index].row_scroll[y];
#ifdef NTM_INTRO_PLANAR_WORDS
                if(nr_scene_row_planar_words(&current,y,&target,&composition)<0){report[2]=0x45525221;return;}
#else
                if(nr_scene_row_cached(&current,y,row,&composition)<0 || nr_store_scene_row_packed(&target,y,row)<0){report[2]=0x45525221;return;}
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
