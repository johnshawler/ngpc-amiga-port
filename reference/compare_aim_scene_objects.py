#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private exact oracle for the opening aim scene's ten object contracts.

No cartridge bytes, extracted tables, or captured graphics are embedded here.
Uses the original2007ca/201192/200c7b/20067c/200aa6 implementations as oracle.
"""
import hashlib
import compare_tee_objects as O
from aim_object_assets import resolve_scene,SCENE_SELECTIONS
from compare_tee_objects import C,Animation,Assets,Native,store,load,reg,execute,data,R,buf

r=resolve_scene(load,data)
O.shape_root=r['shape_address'];O.tile_root=r['tile_address']
O.shape_end=len(r['shapes']);O.tile_end=len(r['tiles'])
O.shape_buffer=buf(r['shapes']);O.tile_buffer=buf(r['tiles'])
buffers=[];animations=(Animation*40)()
for velocity,(raw,frames) in r['animations'].items():
    b=buf(raw);buffers.append(b);animations[velocity//4]=Animation(b,len(b),frames)
O.assets=Assets(animations,O.shape_buffer,O.tile_buffer,O.shape_end,O.tile_end)
O.asset_refs=(C.POINTER(Assets)*32)(*[C.pointer(O.assets) for _ in range(32)])

# IDs/tiles emitted by209f5c/20a170/20a214 and child20aee1.
descriptors=((0,1,1),(1,2,59),(2,3,60),(3,4,61),(5,6,5),(6,7,7),
             (7,8,62),(8,9,8),(9,10,10),(10,11,12),(17,1,206))
cases=rounds=0

def object_state(i,tag,tile,x,y,flags=128,priority=0,color=0):
    p=0x453b+64*i
    for off,value,size in ((1,priority,1),(2,tag,2),(4,x,2),(6,y,2),(8,56,2),
            (10,SCENE_SELECTIONS[tag],2),(14,color,1),
            (15,load(0x219750+5*tag+4,1),1),(16,flags,1),(18,tile,2),(30,0,1)):
        store(p+off,value,size)

def register(m,i,label):
    assert m.register(i)==0
    reg(4,0x453b+64*i);execute(0x2007ca);m.compare((label,'register',i))

def change(m,i,field,offset,value,size=1):
    store(0x453b+64*i+offset,value,size)
    if field=='x':m.x[i]=value
    else:setattr(m.c[i],field,value)

# Every genuine selection: active/frozen/hidden, horizontal/vertical flips,
# wrapped and edge coordinates. Full animation rounds include tag2's blink.
for i,tag,tile in descriptors:
    for flags in (128,132,136,140,160,164,168,172,192,224):
        for x,y in ((64,80),(65535,65535),(0,0),(159,151)):
            O.initialize(0);object_state(i,tag,tile,x,y,flags,priority=17,color=tag&15)
            m=Native();register(m,i,(tag,flags,x,y))
            for tick in range(24):
                if tick==8:
                    store(0x400d,m.global_.value|32,1);m.global_.value|=32
                if tick==12:change(m,i,'flags',16,128|(flags&44))
                if tick==20:change(m,i,'flags',16,m.c[i].flags|16)
                O.compare_round(m,(tag,flags,x,y,tick),head=tick%3!=1);rounds+=1
            cases+=1

# Mixed real scene and maximum occupancy, changed priorities, scroll, palette
# banks, capacity clipping, explicit and traversal removals. Registration and
# sprite ordering are compared over all32 original records and both banks.
tags=tuple(SCENE_SELECTIONS)
for case in range(32):
    O.initialize(0)
    if case%2==0:objects=descriptors
    else:objects=tuple((i,tags[i%len(tags)],64+12*i) for i in range(32))
    for i,tag,tile in objects:
        object_state(i,tag,tile,R.choice((0,8,56,104,159,160,65535)),
                     R.choice((0,24,80,151,152,65535)),128|((i%4)<<2),
                     priority=R.choice((0,1,17,255)),color=(case*3+i)&143)
    m=Native()
    for i,_,_ in objects:register(m,i,('mixed',case))
    start=case%64;store(0x66bb,start,1);store(0x66bc,64-start,1)
    m.d.zero_flags[0]=start;m.d.capacity=64-start
    for tick in range(32):
        if tick in (5,15):
            sx=R.randrange(65536);sy=R.randrange(65536)
            store(0x4d80,sx,2);store(0x4d82,sy,2)
            m.w(0x4d80,sx,2);m.w(0x4d82,sy,2)
            m.d.zero_flags[6]=sx&255;m.d.zero_flags[7]=sy&255
            for i,_,_ in objects:
                change(m,i,'x',4,(sx+8*i)&65535,2)
                change(m,i,'y',6,(sy+4*i)&65535,2)
                change(m,i,'second',1,R.choice((0,1,17,255)))
        if tick==16:
            i=objects[0][0]
            assert m.remove(i)==0;reg(4,0x453b+64*i);execute(0x200824)
            m.compare(('mixed',case,'remove'))
        if tick==22:
            for i,_,_ in objects[1:]:change(m,i,'flags',16,128|((case&3)<<2))
        if tick==28:
            for i,_,_ in objects[1:]:change(m,i,'flags',16,m.c[i].flags|16)
        O.compare_round(m,('mixed',case,tick),head=tick%3!=1);rounds+=1
    cases+=1

# Every queue occupancy retains the queued resource identity and exact FIFO
# behavior, even when no room remains for newly changed objects.
shape=int.from_bytes(r['animations'][8][0][4:6],'little')&32767
count=0
while True:
    count+=1
    if r['shapes'][shape+2*count-2]&128:break
for q in range(65):
    O.initialize(0)
    for i,tag,tile in descriptors:object_state(i,tag,tile,64,80)
    m=Native()
    for i,_,_ in descriptors:register(m,i,('queue',q))
    for j in range(q):
        store(0x6000+6*j,count,1);store(0x6001+6*j,100,1)
        store(0x6002+6*j,O.shape_root+shape+2*count,4)
    store(0x6182,q,1);store(0x6180,6*q,2);m=Native()
    for tick in range(3):O.compare_round(m,('queue',q,tick));rounds+=1
    cases+=1

print(O.json.dumps({'cases':cases,'tail_head_rounds':rounds,'index':56,
    'tags':SCENE_SELECTIONS,'second_ball_slot':17,
    'scope':'all nonstack RAM;32 records/list/bounds;both sprite banks and palettes;'
            'FIFO jobs;8192 tile bytes;256 hardware sprite bytes;64 sprite palettes',
    'includes':'restart,freeze,hide,flips,empty blink,scroll,priority,clipping,32 objects,'
               'explicit/swapped removal,queue occupancies0..64,delayed heads',
    'c_source_sha256':hashlib.sha256((O.ROOT/'native-game/tee_objects.c').read_bytes()).hexdigest()}))
