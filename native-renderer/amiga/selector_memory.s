| SPDX-License-Identifier: GPL-2.0-or-later
| 68020 accepts unaligned longword accesses, including 2-mod-4 bitmap rows.
| Generated cache entries are aligned; tests cover all 16 offset combinations.
| Exact bounded copies/comparisons, no overread and no compiler aliasing tricks.
.text
.globl selector_span_copy
selector_span_copy:
 move.l 4(%sp),%a0
 move.l 8(%sp),%a1
 move.l 12(%sp),%d0
 move.l %d0,%d1
 lsr.l #4,%d1
 beq 2f
1:
 move.l (%a1)+,(%a0)+
 move.l (%a1)+,(%a0)+
 move.l (%a1)+,(%a0)+
 move.l (%a1)+,(%a0)+
 subq.l #1,%d1
 bne 1b
2:
 and.l #15,%d0
 beq 4f
3:
 move.b (%a1)+,(%a0)+
 subq.l #1,%d0
 bne 3b
4:
 rts
.globl selector_span_equal
selector_span_equal:
 move.l 4(%sp),%a0
 move.l 8(%sp),%a1
 move.l 12(%sp),%d0
 move.l %d0,%d1
 lsr.l #2,%d1
 beq 6f
5:
 cmpm.l (%a0)+,(%a1)+
 bne 9f
 subq.l #1,%d1
 bne 5b
6:
 and.l #3,%d0
 beq 8f
7:
 cmpm.b (%a0)+,(%a1)+
 bne 9f
 subq.l #1,%d0
 bne 7b
8:
 moveq #1,%d0
 rts
9:
 moveq #0,%d0
 rts
