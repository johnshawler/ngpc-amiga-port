/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_RANDOM_BYTE_H
#define NTM_RANDOM_BYTE_H
#include <stdint.h>
/* Caller supplies the 256-byte asset window centered 128 bytes before the
 * original signed-index table pointer, plus the shared current cursor.
 * No table bytes are shipped here. Returns 0..255, or -1 without mutation for
 * invalid pointers. Animation entry must not reset or privately fork cursor. */
int ntm_random_next(uint8_t *cursor,const uint8_t window[256]);
#endif
