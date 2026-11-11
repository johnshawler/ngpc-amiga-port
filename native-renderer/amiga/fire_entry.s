| SPDX-License-Identifier: GPL-2.0-or-later
| DOS startup and cleanup precede hardware takeover. C ABI preserves d2-d7/a2-a6.
.text
.globl _start,fire_os_open_library,fire_os_close_library
.globl fire_os_open,fire_os_close,fire_os_read,fire_os_rewind,fire_os_write_error
.globl fire_os_avail,fire_os_alloc,fire_os_free
_start:
 movem.l %d2-%d7/%a2-%a6,-(%sp)
 move.l %sp,%a5
 lea fire_stack,%a0
 move.w #4095,%d0
 move.l #0xa5a5a5a5,%d1
0:
 move.l %d1,(%a0)+
 dbra %d0,0b
 lea fire_stack_top,%sp
 jsr fire_boot_main
 move.l %a5,%sp
 movem.l (%sp)+,%d2-%d7/%a2-%a6
 rts
fire_os_open_library:
 move.l %a6,-(%sp)
 move.l 4.w,%a6
 lea dos_name,%a1
 moveq #0,%d0
 jsr -552(%a6)
 move.l %d0,%a0
 move.l (%sp)+,%a6
 rts
fire_os_close_library:
 move.l %a6,-(%sp)
 move.l 8(%sp),%a1
 move.l 4.w,%a6
 jsr -414(%a6)
 move.l (%sp)+,%a6
 rts
fire_os_open:
 movem.l %d2/%a6,-(%sp)
 move.l 12(%sp),%d1
 move.l #1005,%d2
 move.l fire_dos_base,%a6
 jsr -30(%a6)
 movem.l (%sp)+,%d2/%a6
 rts
fire_os_close:
 move.l %a6,-(%sp)
 move.l 8(%sp),%d1
 move.l fire_dos_base,%a6
 jsr -36(%a6)
 move.l (%sp)+,%a6
 rts
fire_os_read:
 movem.l %d2-%d3/%a6,-(%sp)
 move.l 16(%sp),%d1
 move.l 20(%sp),%d2
 move.l 24(%sp),%d3
 move.l fire_dos_base,%a6
 jsr -42(%a6)
 movem.l (%sp)+,%d2-%d3/%a6
 rts
fire_os_rewind:
 movem.l %d2-%d3/%a6,-(%sp)
 move.l 16(%sp),%d1
 moveq #0,%d2
 moveq #-1,%d3
 move.l fire_dos_base,%a6
 jsr -66(%a6)
 movem.l (%sp)+,%d2-%d3/%a6
 rts
fire_os_write_error:
 movem.l %d2-%d3/%a6,-(%sp)
 move.l fire_dos_base,%a6
 jsr -60(%a6)
 move.l %d0,%d1
 move.l 16(%sp),%d2
 move.l 20(%sp),%d3
 jsr -48(%a6)
 movem.l (%sp)+,%d2-%d3/%a6
 rts
fire_os_avail:
 move.l %a6,-(%sp)
 move.l 4.w,%a6
 moveq #2,%d1
 jsr -216(%a6)
 move.l (%sp)+,%a6
 rts
fire_os_alloc:
 move.l %a6,-(%sp)
 move.l 8(%sp),%d0
 move.l #0x10002,%d1
 move.l 4.w,%a6
 jsr -198(%a6)
 move.l %d0,%a0
 move.l (%sp)+,%a6
 rts
fire_os_free:
 move.l %a6,-(%sp)
 move.l 8(%sp),%a1
 move.l 12(%sp),%d0
 move.l 4.w,%a6
 jsr -210(%a6)
 move.l (%sp)+,%a6
 rts
.balign 2
dos_name: .asciz "dos.library"

.bss
.balign 4
.globl fire_stack
fire_stack: .space 16384
fire_stack_top:
