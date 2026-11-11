/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SHOT_FRONTEND_H
#define SHOT_FRONTEND_H
#include "selector_frontend.h"
#include "../../native-game/shot_raster.h"
typedef struct {uint8_t raw,latched,plan[152];uint32_t frames;} shot_rows;
/* Two source frames per logical update, beginning with VBlank152..198.
 * Only repeated no-change row spans coalesce. Exact152-row output retained. */
int shot_rows_update(shot_rows *,ntm_shot_raster *,uint8_t direct_scroll);
int shot_rows_uniform(const nr_scene *,const uint8_t rows[152]);
void shot_dirty_rows(const nr_scene *,const uint8_t old_rows[152],const nr_scene *,
    const uint8_t new_rows[152],uint8_t dirty[152],void (*pump)(void));
#endif
