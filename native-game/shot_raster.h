/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_RASTER_H
#define NTM_SHOT_RASTER_H
#include <stdint.h>
/* Bounded normal banner-exit service: original20c620 callback, Timer0 divisor1.
 * lower/upper are owned bytes4441/4443, not cartridge resources. callback0
 * represents the original immediate-RETI handler200471;1 is banner20c620.
 * timer_run/mode/divisor and gpu_control model the relevant hardware writes.
 * gpu_control accepts only its readable hardware bits6/7; other bits reject.
 * This is not a CPU, general interrupt scheduler, or timer-frequency emulator. */
typedef struct {
    uint8_t lower_scroll,upper_scroll;
    uint8_t timer_run,timer_mode,timer_divisor,gpu_control,callback,counter;
    uint8_t interrupt_shadow; /* Existing game byte6c27; see remove contract. */
} ntm_shot_raster;
/*20042e with A=1, then200453. No eager scroll write or invented wait. Both
 * preserve source bytes and unrelated control bits; removal leaves mode/divisor.
 * The original timer counter is reset by stopping Timer0 in either routine.
 * Install preserves6c27; removal clears its low nibble through21964b and
 * disables Timer0 priority. The high Timer1 shadow nibble remains untouched. */
int ntm_shot_raster_install(ntm_shot_raster *);
int ntm_shot_raster_remove(ntm_shot_raster *);
/* Original callback reads the CURRENT raster counter. Source bytes are read at
 * dispatch time. Only raw scroll1x changes; all other scene layers stay put. */
int ntm_shot_raster_irq(const ntm_shot_raster *,uint8_t raster_line,uint8_t *raw_scroll);
/* One original scanline boundary, in latch/draw/counter/IRQ order. Call for all
 * lines0..screen_period, including blanking; period must be151..255. HINT latches
 * and dispatches on lines0..150 and screen_period, not151..period-1. The shown
 * byte is the latched scroll for rendered_line; callback sees the NEXT line.
 * Timer0 HINT must be serviced before the following line, as on this bounded
 * normal path. Pointers/state must be disjoint. Validation precedes mutation.
 * A frame-end scalar does not substitute for these row-local outputs. */
int ntm_shot_raster_step(ntm_shot_raster *,unsigned rendered_line,unsigned screen_period,
    uint8_t *raw_scroll,uint8_t *latched_scroll,uint8_t *shown_scroll);
#endif
