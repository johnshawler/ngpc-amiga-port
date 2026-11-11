/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FIRE_STREAM_H
#define FIRE_STREAM_H
#include <stddef.h>
#include <stdint.h>
typedef struct {uint32_t raw_bytes,packed_bytes,checksum;} fire_pack_spec;
typedef struct {
    void *context;
    /* May return a positive short read. Zero is EOF, negative is an I/O error. */
    long (*read)(void *,uint8_t *,size_t);
    int (*rewind)(void *);
} fire_stream;
enum {FIRE_PACK_ARGUMENT=-1,FIRE_PACK_IO=-2,FIRE_PACK_FORMAT=-3,FIRE_PACK_CHECKSUM=-4};
/* The stream has a 20-byte big-endian NTMFPK1 header, then the exact aim LZ
 * grammar. Header sizes/checksum must equal the independently compiled spec.
 * First pass validates grammar, checksum and EOF without touching output.
 * Second pass decodes, rechecks everything and scrubs output on any failure.
 * Caller must publish no asset bindings before success. No heap allocation;
 * maximum buffered compressed bytes=512. The callback must not alias output.
 * FNV-1a detects accidental corruption; this is not a cryptographic signature. */
int fire_stream_unpack(const fire_stream *,const fire_pack_spec *,uint8_t *,size_t);
#endif
