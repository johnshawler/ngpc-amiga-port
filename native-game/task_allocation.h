/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_TASK_ALLOCATION_H
#define NTM_TASK_ALLOCATION_H
#include "task_lifecycle.h"
/* Native free-list terminator is NULL. On exhaustion return the caller's
 * reserved fallback object, exactly as the original returns its reserved node.
 * Valid inputs require a non-null reserved fallback, disjoint free/active lists,
 * and coherent caller-owned nodes. No OS allocation occurs.
 * The fallback is not linked, initialized or invoked by this operation. */
ntm_task *ntm_task_allocate(ntm_task_context *context,ntm_task *parent,
                          ntm_task_callback callback,ntm_task *fallback);
/* Remove consecutive deeper-priority descendants, preserving current parent.
 * Pool nodes remain caller-owned. current==parent and a valid terminating list
 * are required; depth is the original wrapping unsigned byte priority. */
int ntm_task_remove_descendants(ntm_task_context *context,ntm_task *parent);
#endif
