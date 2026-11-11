/* Input-injecting native selector/title/round-record emulator verifier host. GPL-2.0-or-later. */
#define _POSIX_C_SOURCE 200809L
#include <libretro.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
static const char *directory;
static unsigned format,width,height;
static uint8_t *pixels;
static void *chip;
static size_t chip_size;
static unsigned buttons, marker, frame_number, stage, stable_frames, last_present;
static unsigned stage_update, stage_frame;
static const char *screen_path,*chip_path,*scenario;
static uint32_t word(unsigned n){const uint8_t *p=(const uint8_t *)chip+marker+4*n;return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static void capture(const char *label){
 char name[2048];FILE *f;
 snprintf(name,sizeof name,"%s.%s.ppm",screen_path,label);f=fopen(name,"wb");if(!f)abort();
 fprintf(f,"P6\n%u %u\n255\n",width,height);if(fwrite(pixels,3,(size_t)width*height,f)!=(size_t)width*height)abort();fclose(f);
 snprintf(name,sizeof name,"%s.%s.bin",chip_path,label);f=fopen(name,"wb");if(!f)abort();if(fwrite(chip,1,chip_size,f)!=chip_size)abort();fclose(f);
 printf("STAGE %s frame=%u update=%u selection=%u shown=%u dropped=%u\n",label,frame_number,word(7),word(8),word(21),word(12));fflush(stdout);
}
static unsigned navigation, press_frame, first_report_frame;
static unsigned accept_frame, first_scenery_report_frame, scenery_captured;
static unsigned captured_update, skip_stage, skip_update;
static unsigned round_skip_stage, round_skip_update;
static unsigned tee_skip_stage, tee_skip_update;
static void title_input(void){
 if(strncmp(scenario,"skip",4))return;
 if(skip_stage==0 && word(61)==1 && word(7)>=word(58)+8){
  buttons=1u<<RETRO_DEVICE_ID_JOYPAD_B;skip_update=word(7);skip_stage=1;
  printf("TITLE_SKIP input_frame=%u update=%u phase=%u\n",frame_number,word(7),word(61));
 }else if(skip_stage==1 && word(7)>=skip_update+5){buttons=0;skip_stage=2;}
 else if(skip_stage==2 && !strcmp(scenario,"skipboth") && word(61)==3 && word(7)>=word(59)+8){
  buttons=1u<<RETRO_DEVICE_ID_JOYPAD_B;skip_update=word(7);skip_stage=3;
  printf("TOURNAMENT_SKIP input_frame=%u update=%u phase=%u\n",frame_number,word(7),word(61));
 }else if(skip_stage==3 && word(7)>=skip_update+5){buttons=0;skip_stage=4;}
}
static void round_input(void){
 if(strncmp(scenario,"recordskip",10) && strcmp(scenario,"recordheld"))return;
 if(!round_skip_stage && word(65) && !word(66) && word(7)>=word(65)+8){
  buttons=1u<<RETRO_DEVICE_ID_JOYPAD_B;round_skip_update=word(7);round_skip_stage=1;
  printf("ROUND_SKIP input_frame=%u update=%u phase=%u\n",frame_number,word(7),word(67));
 }else if(round_skip_stage==1 && strcmp(scenario,"recordheld") && word(7)>=round_skip_update+5){
  buttons=0;round_skip_stage=2;
 }
}
static void tee_input(void){
 if(strncmp(scenario,"teeskip",7) && strncmp(scenario,"teeseen",7) && strcmp(scenario,"teeheld"))return;
 if(!tee_skip_stage && word(75) && !word(76) &&
    (!strncmp(scenario,"teeseen",7)?(word(80)==4 && word(78)==4 && stable_frames>=2):(word(7)>=word(75)+8))){
  buttons=1u<<RETRO_DEVICE_ID_JOYPAD_B;tee_skip_update=word(7);tee_skip_stage=1;
  printf("TEE_SKIP input_frame=%u update=%u phase=%u\n",frame_number,word(7),word(78));
 }else if(tee_skip_stage==1 && strcmp(scenario,"teeheld") && word(7)>=tee_skip_update+5){buttons=0;tee_skip_stage=2;}
}
static unsigned destinations[]={2,1,0,1,2,1,1};
static unsigned navigation_count=6;
static const char *const labels[]={"right","left","japan","return","repeat","return2","chosen"};
static void press_navigation(void){
 unsigned direction=destinations[navigation]>word(8)?RETRO_DEVICE_ID_JOYPAD_RIGHT:RETRO_DEVICE_ID_JOYPAD_LEFT;
 buttons=1u<<direction;stage_update=word(7);press_frame=frame_number;first_report_frame=0;
 printf("INPUT %s frame=%u target=%u\n",labels[navigation],frame_number,destinations[navigation]);
}
static void advance(void){
 if(!chip||chip_size!=2097152)return;
 if(!marker){for(unsigned n=4;n+512<chip_size;n+=4)if(!memcmp((const uint8_t *)chip+n,"NTEE0001",8)){marker=n;break;}}
 if(!marker||word(2)!=0x52454144)return;
 unsigned present=word(13);if(present==last_present)stable_frames++;else stable_frames=0;last_present=present;
 title_input();round_input();tee_input();
 if(accept_frame && stable_frames>=2 && captured_update!=word(19)){
  char label[64];snprintf(label,sizeof label,"intro-%03u",word(19));capture(label);captured_update=word(19);
  printf("PRESENTED_INTRO label=%s frame=%u snapshot_update=%u current_update=%u title_phase=%u live_title_phase=%u round_phase=%u live_round_phase=%u tee_phase=%u live_tee_phase=%u composition_ticks=%u\n",label,frame_number,word(19),word(7),word(62),word(61),word(68),word(67),word(80),word(78),word(14));
 }
 if(accept_frame && word(54) && word(19)>=word(54) && !word(62) && ((const uint8_t *)chip)[word(27)+8]==160){
  if(!first_scenery_report_frame)first_scenery_report_frame=frame_number;
  if(!scenery_captured && stable_frames>=2){
   capture("scenery");scenery_captured=1;
   printf("INTRO_LATENCY input_frame=%u first_scenery_report_frame=%u verified_scenery_frame=%u reported_frames=%u verified_frames=%u snapshot_update=%u current_update=%u first_visible_update=%u running=%u composition_ticks=%u\n",accept_frame,first_scenery_report_frame,frame_number,first_scenery_report_frame-accept_frame,frame_number-accept_frame,word(19),word(7),word(54),word(9),word(14));
  }
 }
 if((stage==1 || stage==2) && !first_report_frame && word(21)==destinations[navigation])first_report_frame=frame_number;
 if(stage==0 && word(19)>=8 && stable_frames>=2){capture("initial");press_navigation();stage=1;}
 else if(stage==1 && word(7)>=stage_update+5){buttons=0;stage=2;}
 else if(stage==2 && word(21)==destinations[navigation] && stable_frames>=2){
  capture(labels[navigation]);
  printf("LATENCY %s input_frame=%u first_report_frame=%u verified_frame=%u reported_frames=%u verified_frames=%u\n",labels[navigation],press_frame,first_report_frame,frame_number,first_report_frame-press_frame,frame_number-press_frame);
  navigation++;
  if(navigation<navigation_count){press_navigation();stage=1;}
  else{
   buttons=1u<<(strcmp(scenario,"cancel")?RETRO_DEVICE_ID_JOYPAD_B:RETRO_DEVICE_ID_JOYPAD_A);
   if(!strcmp(scenario,"random"))buttons|=1u<<RETRO_DEVICE_ID_JOYPAD_UP;
   accept_frame=frame_number;stage_update=word(7);stage=5;
  }
 }
 else if(stage==5 && word(7)>=stage_update+5){buttons=0;stage=6;}
 else if(stage==6 && !word(9)){stage_frame=frame_number;stage=7;}
 else if(stage==7 && frame_number>stage_frame+400 && stable_frames>=2){capture("finished");stage=8;}
}
static struct {char *key,*value;} options[512];
static unsigned option_count;
static void log_message(enum retro_log_level level,const char *fmt,...){
 (void)level;va_list ap;va_start(ap,fmt);vfprintf(stderr,fmt,ap);va_end(ap);
}
static bool environment(unsigned cmd,void *data){
 switch(cmd){
 case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
 case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
 case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:*(const char **)data=directory;return true;
 case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:((struct retro_log_callback *)data)->log=log_message;return true;
 case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:*(unsigned *)data=0;return true;
 case RETRO_ENVIRONMENT_GET_CAN_DUPE:*(bool *)data=true;return true;
 case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:format=*(unsigned *)data;return format==RETRO_PIXEL_FORMAT_XRGB8888||format==RETRO_PIXEL_FORMAT_RGB565;
 case RETRO_ENVIRONMENT_SET_VARIABLES:{
  const struct retro_variable *v=data;
  for(;v->key;v++){
   if(option_count>=512)return false;
   options[option_count].key=strdup(v->key);
   const char *p=strchr(v->value,';');p=p?p+2:v->value;
   options[option_count].value=strdup(p);
   char *end=strchr(options[option_count].value,'|');if(end)*end=0;
   option_count++;
  }return true;
 }
 case RETRO_ENVIRONMENT_GET_VARIABLE:{
  struct retro_variable *v=data;
  if(!strcmp(v->key,"puae_model")){v->value="A1200OG";return true;}
  if(!strcmp(v->key,"puae_fastmem_size")){v->value="0";return true;}
  for(unsigned i=0;i<option_count;i++)if(!strcmp(v->key,options[i].key)){v->value=options[i].value;return true;}
  v->value=NULL;return false;
 }
 case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:*(bool *)data=false;return true;
 case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:{
  const struct retro_memory_map *m=data;
  for(unsigned i=0;i<m->num_descriptors;i++){
   const struct retro_memory_descriptor *d=m->descriptors+i;
   if(d->addrspace&&!strcmp(d->addrspace,"CHIP")){chip=(uint8_t *)d->ptr+d->offset;chip_size=d->len;}
  }return true;
 }
 case RETRO_ENVIRONMENT_GET_LANGUAGE:*(unsigned *)data=RETRO_LANGUAGE_ENGLISH;return true;
 case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
 case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
 case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
 case RETRO_ENVIRONMENT_SET_GEOMETRY:
 case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:return true;
 default:return false;
 }
}
static void video(const void *data,unsigned w,unsigned h,size_t pitch){
 if(!data)return;
 width=w;height=h;pixels=realloc(pixels,(size_t)w*h*3);if(!pixels)abort();
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  unsigned r,g,b;const uint8_t *row=(const uint8_t *)data+y*pitch;
  if(format==RETRO_PIXEL_FORMAT_XRGB8888){uint32_t v;memcpy(&v,row+x*4,4);r=v>>16&255;g=v>>8&255;b=v&255;}
  else {uint16_t v;memcpy(&v,row+x*2,2);r=(v>>11)*255/31;g=((v>>5)&63)*255/63;b=(v&31)*255/31;}
  size_t off=((size_t)y*w+x)*3;pixels[off]=r;pixels[off+1]=g;pixels[off+2]=b;
 }
}
static void sample(int16_t l,int16_t r){(void)l;(void)r;}
static size_t batch(const int16_t *p,size_t n){(void)p;return n;}
static void poll(void){}
static int16_t input(unsigned p,unsigned d,unsigned i,unsigned b){(void)i;if(d!=RETRO_DEVICE_JOYPAD || p!=0)return 0;if(b==RETRO_DEVICE_ID_JOYPAD_MASK)return (int16_t)buttons;return b<16 && (buttons&(1u<<b))?1:0;}
#define FN(ret,name,args) ret (*name)args=dlsym(lib,#name);if(!name){fprintf(stderr,"Missing %s\n",#name);return 2;}
int main(int argc,char **argv){
 if(argc!=8){fprintf(stderr,"usage: host core adf system-dir frames screen.ppm chip.bin course0|course1|course2|cancel|random|skip0|skip1|skip2|skipboth|recordskip0|recordskip1|recordskip2|recordheld|teeskip0|teeskip1|teeskip2|teeheld|teeseen0|teeseen1|teeseen2\n");return 2;}
 void *lib=dlopen(argv[1],RTLD_NOW);if(!lib){fprintf(stderr,"%s\n",dlerror());return 2;}directory=argv[3];screen_path=argv[5];chip_path=argv[6];scenario=argv[7];
 if(!strcmp(scenario,"course1") || (!strcmp(scenario,"skip1") || !strcmp(scenario,"recordskip1") || !strcmp(scenario,"teeskip1") || !strcmp(scenario,"teeseen1"))){destinations[6]=2;navigation_count=7;}
 else if(!strcmp(scenario,"course2") || (!strcmp(scenario,"skip2") || !strcmp(scenario,"recordskip2") || !strcmp(scenario,"teeskip2") || !strcmp(scenario,"teeseen2"))){destinations[6]=0;navigation_count=7;}
 FN(void,retro_set_environment,(retro_environment_t));
 FN(void,retro_set_video_refresh,(retro_video_refresh_t));
 FN(void,retro_set_audio_sample,(retro_audio_sample_t));
 FN(void,retro_set_audio_sample_batch,(retro_audio_sample_batch_t));
 FN(void,retro_set_input_poll,(retro_input_poll_t));
 FN(void,retro_set_input_state,(retro_input_state_t));
 FN(void,retro_set_controller_port_device,(unsigned,unsigned));
 FN(void,retro_init,(void));FN(void,retro_run,(void));FN(void,retro_unload_game,(void));FN(void,retro_deinit,(void));
 FN(bool,retro_load_game,(const struct retro_game_info *));
 retro_set_environment(environment);retro_set_video_refresh(video);retro_set_audio_sample(sample);retro_set_audio_sample_batch(batch);retro_set_input_poll(poll);retro_set_input_state(input);retro_init();
 struct retro_game_info game={0};game.path=argv[2];
 if(!retro_load_game(&game)){fprintf(stderr,"Load rejected\n");return 3;}
 retro_set_controller_port_device(0,RETRO_DEVICE_JOYPAD);
 unsigned frames=(unsigned)strtoul(argv[4],NULL,10);if(!frames||frames>10000)return 4;
 for(frame_number=0;frame_number<frames && stage<8;frame_number++){retro_run();advance();}
 if(!pixels||!chip||chip_size!=2097152){fprintf(stderr,"Capture/2MiB Chip map missing\n");return 5;}
 FILE *f=fopen(argv[5],"wb");if(!f)return 6;fprintf(f,"P6\n%u %u\n255\n",width,height);
 if(fwrite(pixels,3,(size_t)width*height,f)!=(size_t)width*height)return 7;
 fclose(f);f=fopen(argv[6],"wb");if(!f)return 6;if(fwrite(chip,1,chip_size,f)!=chip_size)return 7;fclose(f);
 printf("CAPTURE frames=%u width=%u height=%u chip_bytes=%zu\n",frames,width,height,chip_size);
 printf("TEE stage=%u actual_frames=%u\n",stage,frame_number);retro_unload_game();retro_deinit();free(pixels);
 for(unsigned i=0;i<option_count;i++){free(options[i].key);free(options[i].value);}
 dlclose(lib);return stage==8?0:8;
}
