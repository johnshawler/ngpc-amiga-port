#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
from shot_scene_objects_case_common import *
class ClubTable(C.Structure):_fields_=[('data',P),('size',C.c_size_t)]
class ClubResources(C.Structure):
 _fields_=[('aim',P),('scene',P),('powers',ClubTable*12),('lies',(ClubTable*15)*12),('overflow_lies',(ClubTable*15)*12),('labels',Asset*23),('digits',Asset*11),('lie_icons',Asset*19),('highlights',Asset*23)]
class Club(C.Structure):
 _fields_=[('wait',P)]+[(n,U8) for n in ('maximum','minimum','label','delta','restore_delay','change_flag','phase')]
club_resources=ClubResources();club_resources.aim=addr(AM.aim_resources);club_resources.scene=addr(scene_object_resources)
for p in range(12):
 club_resources.powers[p]=ClubTable(scene_span(load(0x21eeb7+4*p,4),56),56)
 root=load(0x221838+4*p,4)
 for terrain in range(15):
  club_resources.lies[p][terrain]=ClubTable(scene_span(load(root+4*terrain,4),42),42)
  club_resources.overflow_lies[p][terrain]=ClubTable(scene_span(load(root+4*terrain,4)+768,42),42)
for label in range(23):club_resources.labels[label]=tee_asset(0xb4,load(0x21b800+label,1))
for digit in range(11):club_resources.digits[digit]=tee_asset(0xb4,4*(digit+20))
for icon in range(19):club_resources.lie_icons[icon]=tee_asset(0x9c,288+4*icon)

for label in range(23):club_resources.highlights[label]=tee_asset(0xac,36+load(0x21b817+label,1))
