# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve caller-owned first-shot HUD art without embedding extracted bytes."""
from tee_setup_case_common import *
class HudResources(C.Structure):
 _fields_=[('font',Asset),('header',Asset),('label',Asset),('wind',Asset*17),('label_id',U8)]
class HudContext(C.Structure):
 _fields_=[(n,P) for n in ('screen','maps','tiles','text_base','scratch','saved')]
hud_resources=HudResources();hud_resources.font=tee_asset(0xb4,0,True)
hud_resources.header=tee_asset(0x68,0x3c);hud_resources.label=tee_asset(0x9c,20);hud_resources.label_id=4
for i in range(17):hud_resources.wind[i]=tee_asset(0x9c,4*(28+i))
