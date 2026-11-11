/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SELECTOR_MEMORY_H
#define SELECTOR_MEMORY_H
void selector_span_copy(void *dst,const void *src,unsigned bytes);
int selector_span_equal(const void *a,const void *b,unsigned bytes);
#endif
