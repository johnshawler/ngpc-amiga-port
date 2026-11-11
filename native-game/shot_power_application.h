/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NTM_SHOT_POWER_APPLICATION_H
#define NTM_SHOT_POWER_APPLICATION_H
#include "shot_fire_application.h"
#include "shot_power.h"
#include "shot_power_meter.h"
#include "shot_accuracy.h"
typedef struct {
    const ntm_shot_fire_application_resources *fire;
    const ntm_shot_power_meter_resources *meter;
} ntm_shot_power_application_resources;
typedef struct {
    ntm_shot_fire_application base;
    ntm_shot_power power;
    ntm_shot_power_meter meter;
    ntm_shot_accuracy accuracy;
    ntm_shot_power_context context;
    const ntm_shot_power_application_resources *resources;
    uint32_t power_update;
    uint8_t enabled,boundary;
} ntm_shot_power_application;
enum {NTM_POWER_APPLICATION_WORLD=1,NTM_POWER_APPLICATION_SWING=2,
    NTM_POWER_APPLICATION_METER=4};
/* Opt-in after opening enable, instead of fire/control enable. Preserves the
 * existing fire application and target unchanged. Owns real meter child and
 * two original cancellations; world reload and swing transition remain explicit
 * partial-frame boundaries. All memory and resource spans remain caller-owned. */
int ntm_shot_power_application_enable(ntm_shot_power_application *,
    const ntm_shot_power_application_resources *);
int ntm_shot_power_application_update(ntm_shot_power_application *,uint8_t buttons);
#endif
