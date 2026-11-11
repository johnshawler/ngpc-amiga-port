/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_ROUND_PREPARE_H
#define NTM_ROUND_PREPARE_H
#include <stdint.h>
typedef struct {
    uint8_t players[2][64]; /* Original player records4149/4189, preserved gaps. */
    uint8_t course_order[36]; /* Original441d: (course,hole) byte pairs. */
    uint8_t course_record_flags[3]; /* Caller-resolved saved5847+66*course bytes. */
    uint8_t active_player,mapped_course,course_order_kind,reset_kind;
    uint8_t round_index,hole_index;
} ntm_round_prepare_state;
/* Original204a88: reset round fields/scores, construct order, select record flag.
 * Bounded to course_order_kind4209==0 (five distinct random holes) and course0..2.
 * The RNG cursor is SHARED with the application, not reset or privately forked.
 * window follows random_byte.h's signed-index256-byte caller-owned contract.
 * Returns0 on success; -1 without mutation for bad pointers/unsupported order
 * or a window with fewer than five distinct values modulo18. No mode is read
 * by this helper. The enclosing lifecycle has its own supported-mode contract.
 * Repeating intentionally reruns preparation and advances the RNG again.
 */
int ntm_round_prepare(ntm_round_prepare_state *state,uint8_t *random_cursor,
    const uint8_t random_window[256]);
#endif
