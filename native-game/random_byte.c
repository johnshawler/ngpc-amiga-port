/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "random_byte.h"
int ntm_random_next(uint8_t *cursor,const uint8_t window[256])
{
    unsigned value;if(!cursor || !window)return -1;
    value=window[*cursor^128u];*cursor=(uint8_t)(*cursor+1u);return (int)value;
}
