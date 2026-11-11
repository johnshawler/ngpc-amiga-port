/* SPDX-License-Identifier: GPL-2.0-or-later. Portable test implementation. */
#include "selector_memory.h"
#ifndef SELECTOR_FAST_MEMORY
void selector_span_copy(void *dst,const void *src,unsigned bytes)
{unsigned char *d=dst;const unsigned char *s=src;while(bytes--)*d++=*s++;}
int selector_span_equal(const void *left,const void *right,unsigned bytes)
{const unsigned char *a=left,*b=right;while(bytes--)if(*a++!=*b++)return 0;return 1;}
#endif
