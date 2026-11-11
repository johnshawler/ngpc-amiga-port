/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FIRE_BOOT_H
#define FIRE_BOOT_H
#include "fire_stream.h"
void *fire_os_open_library(void);
void fire_os_close_library(void *);
long fire_os_open(const char *);
long fire_os_close(long);
long fire_os_read(long,uint8_t *,size_t);
long fire_os_rewind(long);
long fire_os_write_error(const char *,size_t);
uint32_t fire_os_avail(void);
uint8_t *fire_os_alloc(uint32_t);
void fire_os_free(uint8_t *,uint32_t);
extern void *fire_dos_base;
int fire_load_pool(const char *,const fire_pack_spec *,uint8_t *,size_t);
uint8_t *fire_boot_prepare(const fire_pack_spec *,uint8_t *,size_t,
    const fire_pack_spec *,uint8_t *,size_t,uint32_t memory[2]);
#endif
