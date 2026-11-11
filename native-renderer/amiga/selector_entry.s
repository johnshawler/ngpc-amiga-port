| SPDX-License-Identifier: GPL-2.0-or-later
.text
.globl _start
_start:
 move.l 4.w,%a6
 move.l #2,%d1
 jsr -216(%a6)
 move.l %d0,%d2
 move.l #170000,%d0
 move.l #0x10002,%d1
 jsr -198(%a6)
 tst.l %d0
 beq 1f
 move.l %d0,%a2
 move.l %d2,160(%a2)
 move.l #2,%d1
 jsr -216(%a6)
 move.l %d0,164(%a2)
 move.l %a2,-(%sp)
 jsr selector_main
 addq.l #4,%sp
1:
 bra 1b
