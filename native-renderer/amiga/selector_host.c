/* Input-injecting native selector emulator verifier host. GPL-2.0-or-later. */
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
static void advance(void){
 if(!chip||chip_size!=2097152)return;
 if(!marker){for(unsigned n=4;n+192<chip_size;n+=4)if(!memcmp((const uint8_t *)chip+n,"NSEL0001",8)){marker=n;break;}}
 if(!marker||word(2)!=0x52454144)return;
 unsigned present=word(13);if(present==last_present)stable_frames++;else stable_frames=0;last_present=present;
 if(stage==0 && word(19)>=8 && stable_frames>=2){capture("initial");buttons=1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT;stage_update=word(7);stage=1;}
 else if(stage==1 && word(7)>=stage_update+5){buttons=0;stage=2;}
 else if(stage==2 && word(21)==2 && stable_frames>=2){capture("right");buttons=1u<<RETRO_DEVICE_ID_JOYPAD_LEFT;stage_update=word(7);stage=3;}
 else if(stage==3 && word(7)>=stage_update+5){buttons=0;stage=4;}
 else if(stage==4 && word(21)==1 && stable_frames>=2){capture("left");buttons=1u<<(strcmp(scenario,"cancel")?RETRO_DEVICE_ID_JOYPAD_B:RETRO_DEVICE_ID_JOYPAD_A);if(!strcmp(scenario,"random"))buttons|=1u<<RETRO_DEVICE_ID_JOYPAD_UP;stage_update=word(7);stage=5;}
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
 if(argc!=8){fprintf(stderr,"usage: host core adf system-dir frames screen.ppm chip.bin accept|cancel|random\n");return 2;}
 void *lib=dlopen(argv[1],RTLD_NOW);if(!lib){fprintf(stderr,"%s\n",dlerror());return 2;}directory=argv[3];screen_path=argv[5];chip_path=argv[6];scenario=argv[7];
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
 printf("SELECTOR stage=%u actual_frames=%u\n",stage,frame_number);retro_unload_game();retro_deinit();free(pixels);
 for(unsigned i=0;i<option_count;i++){free(options[i].key);free(options[i].value);}
 dlclose(lib);return stage==8?0:8;
}
