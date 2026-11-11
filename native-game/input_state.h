/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_INPUT_STATE_H
#define NTM_INPUT_STATE_H
#include <stdint.h>
/* State-level replacement for the game's eight-button update. Bytes:
 * 0 current, 1 rising edges, 2 periodic mask, 3 signed-comparison mask,
 * 4 previous, 5 counter mask, 6 signed threshold, 7 reserved, 8..15 counters.
 * sample is the game's eight-bit input value, not a host key code.
 * Configuration and reserved bytes are preserved. No allocation, ROM access,
 * TLCS registers, Amiga OS or emulated peripherals are used. */
int ntm_input_update(uint8_t state[16],uint8_t sample);
#endif
