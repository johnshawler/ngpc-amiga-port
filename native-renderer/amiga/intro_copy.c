/* SPDX-License-Identifier: GPL-2.0-or-later
 * Generated asset spans and application VRAM are distinct allocations in this
 * private target. Use the already tested 68020 bounded copy without repeatedly
 * decoding a tile offset for each byte. No global scheduler work occurs here. */
#include "selector_memory.h"
void ntm_course_screen_copy_private(void *dst,const void *src,unsigned bytes)
{selector_span_copy(dst,src,bytes);}
