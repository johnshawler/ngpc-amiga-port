/* SPDX-License-Identifier: GPL-2.0-or-later. Native parent-flow microbenchmark with service stubs. */
#include "../../native-game/input_state.c"
#include "../../native-game/course_choice.c"
#include "../../native-game/task_lifecycle.c"
#include "../../native-game/task_allocation.c"
#include "../../native-game/random_byte.c"
#include "../../native-game/course_parent.c"
#define CIA(off) (*(volatile uint8_t *)(0xbfd000u+(off)))
static uint32_t bench_report[112];
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
#define CB(name) static int name(ntm_task_context *c,ntm_task *t){(void)c;(void)t;return 0;}
CB(renderer_cb) CB(wait_cb) CB(cancel_cb) CB(accept_cb) CB(random_cb) CB(cycle_cb) CB(fade_cb) CB(setup_cb)
static void run_benchmark(uint8_t *block,uint8_t *tiles)
{
    unsigned pass,i;uint8_t table[256];uint32_t start,finish,*r=bench_report;
    (void)tiles;
    ((uint32_t *)block)[8]=(uint32_t)bench_report;((uint32_t *)block)[9]=sizeof bench_report;
    ((volatile uint32_t *)block)[10]=1;
    for(i=0;i<256;i++)table[i]=(uint8_t)(i*73+11); /* Authored synthetic asset. */
    CIA(0xe00)=0;CIA(0xf00)=0;CIA(0xd00)=0x7f;
    CIA(0x400)=255;CIA(0x500)=255;CIA(0x600)=255;CIA(0x700)=255;
    CIA(0xf00)=0x51;CIA(0xe00)=0x11;
    while(beam_line()!=0){}start=ticks();while(beam_line()==0){}while(beam_line()!=0){}finish=ticks();
    r[0]=0x4e504642;r[1]=1;r[2]=709379;r[3]=finish-start;
    for(pass=0;pass<6;pass++) {
        unsigned v=pass<3?pass:5-pass,flow,calls=0,draws=0,renders=0,sounds=0,errors=0;
        uint8_t cursor=17;uint32_t checksum=2166136261u;
        ntm_task n[5]={{0}};ntm_task_context c={0};ntm_course_parent_state s={0};
        s.wait_callback=wait_cb;s.cancel_delay_callback=cancel_cb;s.accept_delay_callback=accept_cb;
        s.random_delay_callback=random_cb;s.random_cycle_callback=cycle_cb;s.fade_callback=fade_cb;s.setup_callback=setup_cb;
        ((volatile uint32_t *)block)[10]=10+pass;start=ticks();
        for(flow=0;flow<128;flow++) {
            unsigned local=0;int request;
            c.free_head=n+2;c.current=n;c.selection=1;
            n[0].next=n+1;n[0].previous=n+1;n[1].next=n;n[1].previous=n;
            n[0].priority=n[1].priority=7;n[0].substate=165;n[0].delay=0;
            n[2].next=n+3;n[3].next=0;
            s.global_word=0xaabb;s.global_flags=0x35;s.random_remaining=85;s.mapped_course=204;
            if(ntm_course_parent_initialize(&c,n,renderer_cb,n+4,&s))errors++;
            n[0].result=(uint8_t)(v+1);
            do {
                request=local?ntm_course_parent_tick(&c,n,&s):ntm_course_parent_begin(&c,n,&s);
                while(request>0 && request!=NTM_PARENT_FADE && request!=NTM_PARENT_COURSE_SETUP) {
                    if(request==NTM_PARENT_RANDOM){draws++;request=ntm_course_parent_generate_random(&c,n,&s,&cursor,table);}
                    else {if(request==NTM_PARENT_RENDER)renders++;else sounds++;request=ntm_course_parent_resume(&c,n,&s);}
                }
                local++;calls++;
                if(request<0){errors++;break;}
            }while(request==0 && local<200);
            if(local>=200 || n[0].next!=n+1 || n[1].previous!=n || c.current!=n || c.free_head!=n+2 || n[2].next!=n+3 || n[3].next)errors++;
            if(request!=(v==1?NTM_PARENT_FADE:NTM_PARENT_COURSE_SETUP) || n[0].callback!=(v==1?fade_cb:setup_cb))errors++;
        }
        finish=ticks();
        {uint8_t state[8]={c.selection,cursor,s.random_remaining,s.global_flags,s.mapped_course,n[0].substate,(uint8_t)(n[0].delay>>8),(uint8_t)n[0].delay};
         for(i=0;i<8;i++)checksum=(checksum^state[i])*16777619u;}
        r[4+pass*16]=v;r[5+pass*16]=128;r[6+pass*16]=finish-start;r[7+pass*16]=calls;
        r[8+pass*16]=draws;r[9+pass*16]=renders;r[10+pass*16]=sounds;r[11+pass*16]=checksum;
        r[12+pass*16]=c.selection;r[13+pass*16]=cursor;r[14+pass*16]=s.random_remaining;
        r[15+pass*16]=((uint32_t)s.global_word<<8)|s.global_flags;r[16+pass*16]=n[0].delay;
        r[17+pass*16]=s.mapped_course;r[18+pass*16]=n[0].substate;r[19+pass*16]=errors;
    }
    ((volatile uint32_t *)block)[10]=0x444f4e45;
}
