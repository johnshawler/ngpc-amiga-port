#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private bounded span-size report; emits sizes only, never resource bytes."""
from shot_golfer_case_common import *
from shot_golfer_object_assets import resolve as golfer_objects
from shot_trajectory_object_assets import resolve as trajectory_objects
import json
before_club=len(tee_assets)
import shot_club_case_common
prior=golfer_prior_assets+tee_assets[before_club:]
def aggregate(items):
 descriptors={};tables={}
 for source,a in items:
  descriptors[source]=max(descriptors.get(source,0),a.size)
  pointer=load(source+2,4);tables[pointer]=max(tables.get(pointer,0),a.tiles_size)
 return descriptors,tables
new=[]
for a in golfer_assets:
 source=next(source for source,v in tee_assets if C.cast(v.data,P).value==C.cast(a.asset.data,P).value)
 new.append((source,a.asset))
od,ot=aggregate(prior);nd,nt=aggregate(new)
screen_increment=sum(max(0,size-od.get(source,0)) for source,size in nd.items())+sum(max(0,size-ot.get(source,0)) for source,size in nt.items())
terrain_keep=[];unique={};pairs=[]
for i,g in enumerate(AM.geometries):
 large,ls=resolve_terrain(g.map_index,terrain_keep);close,cs=resolve_terrain(g.aux_index,terrain_keep)
 unique[ls]=max(unique.get(ls,0),large.size);unique[cs]=max(unique.get(cs,0),close.size)
 pairs.append(dict(course=i//18,hole=i%18,large=large.size,close=close.size,total=large.size+close.size))
golfer=golfer_objects(load,data);trajectory=trajectory_objects(load,data)
animations=sum(len(value[0]) for velocity,value in golfer['animations'].items() if velocity not in trajectory['animations'])
object_increment=animations+max(0,len(golfer['shapes'])-len(trajectory['shapes']))+max(0,len(golfer['tiles'])-len(trajectory['tiles']))
print(json.dumps({'terrain_raw_all_pairs':sum(p['total'] for p in pairs),'terrain_unique_sources':len(unique),'terrain_deduplicated_raw':sum(unique.values()),'terrain_largest_pair':max(pairs,key=lambda p:p['total']),'terrain_first_holes':[p for p in pairs if p['hole']==0],
 'golfer_screen_unique_descriptors':len(nd),'golfer_screen_descriptor_bytes':sum(nd.values()),'golfer_screen_unique_tile_tables':len(nt),'golfer_screen_raw_tile_prefix_bytes':sum(nt.values()),'golfer_screen_increment_over_open_assets':screen_increment,
 'tag51_animation_bytes':sum(len(v[0]) for v in golfer['animations'].values()),'tag51_shape_prefix_bytes':len(golfer['shapes']),'tag51_tile_prefix_bytes':len(golfer['tiles']),'tag51_increment_over_tag27':object_increment,
 'scope':'raw span sizes; no structs,code,alignment,compression or DD-fit claim; terrain can be selected per hole rather than all resident'}))
