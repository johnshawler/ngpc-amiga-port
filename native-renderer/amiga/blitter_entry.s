/* SPDX-License-Identifier: GPL-2.0-or-later. */
.text
.globl _start
_start:
 move.l 4.w,%a6
 move.l #300000,%d0
 move.l #0x10002,%d1
 jsr -198(%a6)
 tst.l %d0
 beq 1f
 move.l %d0,-(%sp)
 jsr display_prepare
 addq.l #4,%sp
1:
 bra 1b
