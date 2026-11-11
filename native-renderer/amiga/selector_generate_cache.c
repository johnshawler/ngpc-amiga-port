/* SPDX-License-Identifier: GPL-2.0-or-later
 * Private cache generation from the same native application and supplied assets.
 * No generated cartridge data may be committed. Invoked by selector_build.sh. */
#include "selector_frontend.h"
#include "selector_planar_cache.h"
#include "selector_assets.inc"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static ntm_course_application app;
static ntm_course_resources resources;
static ntm_screen_asset assets[5];
static ntm_palette_job jobs[3];
static uint8_t random_cursor;
static selector_planar_entry entries[15],candidate;
static unsigned live_cursor(const nr_scene *scene)
{
    unsigned i,count=0;for(i=0;i<64;i++)if(scene->sprites[i*4+1]&24)count++;return count;
}
static int verify_traces(void)
{
    unsigned course,mode,checks=0,misses=0;uint8_t check[24320],row[160];
    for(course=0;course<3;course++)for(mode=0;mode<3;mode++){
        unsigned step,normal=0,accept=0;selector_planar_stats stats={0};
        memset(&app,0,sizeof app);random_cursor=0;selector_initialize(&app);app.screen.displayed_selection=(uint8_t)course;
        selector_resources(&resources,assets,jobs,&random_cursor);
        if(ntm_course_application_start(&app,&resources))return 0;
        for(step=0;step<240 && app.driver.running;step++){
            nr_scene scene;int hit;unsigned y;uint8_t buttons=0;
            nr_bitmap bitmap={check,sizeof check,160,152,20};
            /* Mixed history returns to the initial course before acceptance. */
            if(step==35)buttons=(uint8_t)(course==2?4:8);
            if(step==65)buttons=(uint8_t)(course==2?8:4);
            if(step==100)buttons=(uint8_t)(mode==0?16:mode==1?32:80);
            if(ntm_course_application_update(&app,buttons)<0 || ntm_course_application_scene(&app,&scene))return 0;
            hit=selector_planar_find(&scene,entries,15,0,&stats,0);
            if(!scene.window_x && !scene.window_y && scene.window_w==160 && scene.window_h==152 && live_cursor(&scene)==28){
                if(hit<0){fprintf(stderr,"Uncached full selector course%u mode%u step%u\n",course,mode,step);return 0;}
                if((unsigned)hit/5==course){
                    if(app.driver.parent.global_word==4)accept|=1u<<((unsigned)hit%5);
                    else normal|=1u<<((unsigned)hit%5);
                }
            }else if(hit>=0)return 0;else misses++;
            if(hit>=0){
                for(y=0;y<152;y++)if(nr_scene_row(&scene,y,row) || nr_store_scene_row(&bitmap,y,row))return 0;
                if(memcmp(check,entries[hit].planar,sizeof check))return 0;
                checks++;
            }
        }
        if(normal!=31 || (mode==0 && accept!=31) || app.driver.running)return 0;
    }
    printf("Verified %u private baseline frames, normal/accept five-shape coverage, repeated navigation, %u fade/exit fallbacks.\n",checks,misses);
    return 1;
}
static void emit(FILE *f,const uint8_t *p,unsigned n)
{unsigned i;fputs("{",f);for(i=0;i<n;i++){if(!(i%32))fputs("\n",f);fprintf(f,"%u,",p[i]);}fputs("\n}",f);}
int main(int argc,char **argv)
{
    unsigned course,total=0,i,field;FILE *f;
    if(argc!=2)return 2;
    for(course=0;course<3;course++){
        unsigned count=0,step;memset(&app,0,sizeof app);random_cursor=0;
        selector_initialize(&app);app.screen.displayed_selection=(uint8_t)course;
        selector_resources(&resources,assets,jobs,&random_cursor);
        if(ntm_course_application_start(&app,&resources))return 3;
        for(step=0;step<150 && count<5;step++){
            nr_scene scene;selector_planar_stats stats={0};uint8_t row[160];
            nr_bitmap bitmap={candidate.planar,SELECTOR_PLANAR_BYTES,160,152,20};
            if(ntm_course_application_update(&app,0)<0 || ntm_course_application_scene(&app,&scene))return 4;
            if(scene.window_x || scene.window_y || scene.window_w!=160 || scene.window_h!=152)continue;
            if(live_cursor(&scene)!=28)return 9;
            if(count && selector_planar_find(&scene,entries+total-count,count,0,&stats,0)>=0)continue;
            selector_planar_key(&scene,&candidate);
            for(i=0;i<152;i++)if(nr_scene_row(&scene,i,row) || nr_store_scene_row_packed(&bitmap,i,row))return 5;
            entries[total++]=candidate;count++;
        }
        if(count!=5)return 6;
    }
    if(!verify_traces())return 10;
    f=fopen(argv[1],"wb");if(!f)return 7;
    fputs("/* PRIVATE GENERATED PLANAR CACHE. Do not commit or distribute. */\nstatic const selector_planar_entry selector_planar_entries[15]={\n",f);
    for(i=0;i<15;i++){
        const selector_planar_entry *e=entries+i;
        const uint8_t *p[]={e->maps,e->sprites,e->palettes,e->controls,e->referenced,e->tiles,e->planar};
        const unsigned sizes[]={4096,256,64,16,64,8192,SELECTOR_PLANAR_BYTES};
        fputs("{",f);for(field=0;field<7;field++){emit(f,p[field],sizes[field]);fputs(",",f);}fputs("},\n",f);
    }
    fputs("};\n",f);if(fclose(f))return 8;
    printf("Generated 15 exact-key private planar entries, %zu bytes total.\n",sizeof entries);
    return 0;
}
