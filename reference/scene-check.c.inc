/* SPDX-License-Identifier: GPL-2.0-or-later
 * Read-only comparison at the ORIGINAL raster draw point. Test instrumentation
 * only; never compiled into the Amiga game. Counts contain no captured assets. */
#include "scene_renderer.c"
#include "event_renderer.c"
static nr_event_cache event_cache;
static nr_scene_cache sprite_cache; /* static zero initialization = invalid */
static nr_planar_cache planar_cache;
static nr_composition_cache composition_cache;
static uint8_t scalar_bitmap[24320],dirty_bitmap[24320],direct_bitmap[24320],words_bitmap[24320];
static uint64_t scene_rows, scene_pixels, scene_mismatches, scene_skipped;
__attribute__((visibility("default")))
uint64_t retro_reference_scene_count(unsigned n)
{
    switch(n) {
        case 0:return scene_rows; case 1:return scene_pixels;
        case 2:return scene_mismatches; case 3:return scene_skipped;
        case 4:return sprite_cache.rebuilds; case 5:return sprite_cache.hits;
        case 6:return planar_cache.converted_blocks; case 7:return planar_cache.skipped_blocks;
        case 8:return composition_cache.composed_rows; case 9:return composition_cache.reused_rows;
        case 10:return event_cache.composed;case 11:return event_cache.reused;
        case 12:return event_cache.notifications;
        default:return 0;
    }
}
static void reference_check_scene(ngpgfx_t *gfx, const uint16_t *original)
{
    nr_scene s;
    uint8_t pens[160],cached[160],reused[160],event[160];
    unsigned x,p;
    nr_bitmap scalar={scalar_bitmap,sizeof scalar_bitmap,160,152,20};
    nr_bitmap words={words_bitmap,sizeof words_bitmap,160,152,20};
    nr_bitmap direct={direct_bitmap,sizeof direct_bitmap,160,152,20};
    nr_bitmap dirty={dirty_bitmap,sizeof dirty_bitmap,160,152,20};
    if(gfx->K2GE_MODE || gfx->layer_enable!=7) {scene_skipped++;return;}
    s.tiles=gfx->CharacterRAM;s.maps=gfx->ScrollVRAM;
    s.sprites=gfx->SpriteVRAM;s.sprite_pal=gfx->SpriteVRAMColor;
    s.scroll_x[0]=gfx->scroll1x;s.scroll_y[0]=gfx->scroll1y;
    s.scroll_x[1]=gfx->scroll2x;s.scroll_y[1]=gfx->scroll2y;
    s.sprite_x=gfx->scrollsprx;s.sprite_y=gfx->scrollspry;
    s.window_x=gfx->winx;s.window_y=gfx->winy;
    s.window_w=gfx->winw;s.window_h=gfx->winh;
    s.swap=gfx->planeSwap;s.background=gfx->bgc;s.outside=gfx->oowc;
    if(nr_scene_row(&s,gfx->raster_line,pens)) {scene_skipped++;return;}
    if(nr_scene_row_cached(&s,gfx->raster_line,cached,&sprite_cache)) {scene_skipped++;return;}
    for(x=0;x<160;x++)if(cached[x]!=pens[x])scene_mismatches++;
    if(nr_scene_row_reused(&s,gfx->raster_line,reused,&composition_cache)) {scene_skipped++;return;}
    for(x=0;x<160;x++)if(reused[x]!=pens[x])scene_mismatches++;
    if(nr_scene_row_event(&s,gfx->raster_line,event,&event_cache)){scene_skipped++;return;}
    for(x=0;x<160;x++)if(event[x]!=pens[x])scene_mismatches++;
    nr_store_scene_row(&scalar,gfx->raster_line,pens);
    nr_store_scene_row_dirty(&dirty,gfx->raster_line,cached,&planar_cache);
    nr_scene_row_planar(&s,gfx->raster_line,&direct,&sprite_cache);
    nr_scene_row_planar_words(&s,gfx->raster_line,&words,&sprite_cache);
    for(p=0;p<8;p++)for(x=0;x<20;x++) {
        unsigned off=p*3040+gfx->raster_line*20+x;
        if(scalar_bitmap[off]!=dirty_bitmap[off] || scalar_bitmap[off]!=direct_bitmap[off] || scalar_bitmap[off]!=words_bitmap[off])scene_mismatches++;
    }
    scene_rows++;
    for(x=0;x<160;x++) {
        unsigned offset=pens[x]*2;
        unsigned color=gfx->ColorPaletteRAM[offset] | ((unsigned)gfx->ColorPaletteRAM[offset+1]<<8);
        if(gfx->negative)color=~color;
        scene_pixels++;
        if((color&4095)!=(original[x]&4095))scene_mismatches++;
    }
}

static void reference_scene_write(ngpgfx_t *gfx,unsigned address,unsigned data)
{
    if(address>=0xa000 && address<=0xbfff) {
        if(gfx->CharacterRAM[address-0xa000]!=data)nr_event_write(&event_cache,0,address-0xa000);
    }else if(address>=0x9000 && address<=0x9fff) {
        if(gfx->ScrollVRAM[address-0x9000]!=data)nr_event_write(&event_cache,1,address-0x9000);
    }else if(address>=0x8800 && address<=0x88ff) {
        if(gfx->SpriteVRAM[address-0x8800]!=data)nr_event_write(&event_cache,2,0);
    }else if(address>=0x8c00 && address<=0x8c3f) {
        if(gfx->SpriteVRAMColor[address-0x8c00]!=(data&15))nr_event_write(&event_cache,2,0);
    }
}
