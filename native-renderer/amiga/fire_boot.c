/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "fire_boot.h"
void *fire_dos_base;
static long read_file(void *context,uint8_t *data,size_t n){return fire_os_read(*(long *)context,data,n);}
static int rewind_file(void *context){return fire_os_rewind(*(long *)context)<0?-1:0;}
int fire_load_pool(const char *path,const fire_pack_spec *spec,uint8_t *data,size_t n)
{
    long file=fire_os_open(path);int r;fire_stream io;
    if(!file)return FIRE_PACK_IO;
    io.context=&file;io.read=read_file;io.rewind=rewind_file;
    r=fire_stream_unpack(&io,spec,data,n);
    if(!fire_os_close(file) && !r)r=FIRE_PACK_IO;
    return r;
}

uint8_t *fire_boot_prepare(const fire_pack_spec *first,uint8_t *data,size_t n,
    const fire_pack_spec *second,uint8_t *intro,size_t count,uint32_t memory[2])
{
    uint8_t *block;
    static const char failure[]="Native fire: resource load or Chip allocation failed. Hardware unchanged.\n";
    if(!memory)return 0;
    fire_dos_base=fire_os_open_library();if(!fire_dos_base)return 0;
    if(fire_load_pool("fire.pool",first,data,n)<0 || fire_load_pool("intro.pool",second,intro,count)<0){
        fire_os_write_error(failure,sizeof failure-1);fire_os_close_library(fire_dos_base);fire_dos_base=0;return 0;}
    memory[0]=fire_os_avail();block=fire_os_alloc(170000);
    if(!block){fire_os_write_error(failure,sizeof failure-1);fire_os_close_library(fire_dos_base);fire_dos_base=0;return 0;}
    memory[1]=fire_os_avail();fire_os_close_library(fire_dos_base);fire_dos_base=0;
    return block;
}
