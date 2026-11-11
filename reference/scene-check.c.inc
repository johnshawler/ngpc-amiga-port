/* SPDX-License-Identifier: GPL-2.0-or-later
 * Read-only comparison at the ORIGINAL raster draw point. Test instrumentation
 * only; never compiled into the Amiga game. Counts contain no captured assets. */
#include "scene_renderer.c"
static uint64_t scene_rows, scene_pixels, scene_mismatches, scene_skipped;
__attribute__((visibility("default")))
uint64_t retro_reference_scene_count(unsigned n)
{
    switch(n) {
        case 0:return scene_rows; case 1:return scene_pixels;
        case 2:return scene_mismatches; case 3:return scene_skipped;
        default:return 0;
    }
}
static void reference_check_scene(ngpgfx_t *gfx, const uint16_t *original)
{
    nr_scene s;
    uint8_t pens[160];
    unsigned x;
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
    scene_rows++;
    for(x=0;x<160;x++) {
        unsigned offset=pens[x]*2;
        unsigned color=gfx->ColorPaletteRAM[offset] | ((unsigned)gfx->ColorPaletteRAM[offset+1]<<8);
        if(gfx->negative)color=~color;
        scene_pixels++;
        if((color&4095)!=(original[x]&4095))scene_mismatches++;
    }
}
