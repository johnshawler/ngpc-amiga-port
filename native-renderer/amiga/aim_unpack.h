/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef AIM_UNPACK_H
#define AIM_UNPACK_H
#include <stddef.h>
#include <stdint.h>
/* Startup-only lossless LZ stream. Literal0..127: length=opcode+1 bytes.
 * Match128..255: length=(opcode&127)+3, followed by big-endian16 distance.
 * Validates whole stream before any write; exact input/output exhaustion.
 * Source/output must be disjoint. No allocation, dictionary, or per-frame work.
 * Caller owns destination throughout borrowed resource lifetime. */
int aim_unpack(const uint8_t *,size_t,uint8_t *,size_t);
#endif
