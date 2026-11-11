#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Private original hole-preparation resource adapter, not extracted data."""
from screen_probe import *
U8=C.c_uint8;U16=C.c_uint16;U32=C.c_uint32;P=C.c_void_p
class Geometry(C.Structure):
    _fields_=[('descriptor',U8*18),('pins',P),('pins_size',C.c_size_t),('dimensions',U8*2),('map_index',U16),('aux_index',U16)]+[(n,U32) for n in ('descriptor_identity','path_identity','aux_identity')]
class Resources(C.Structure):
    _fields_=[('holes',P),('hole_count',C.c_size_t),('hazards',P),('hazards_size',C.c_size_t),('quarter',P),('quarter_size',C.c_size_t)]+[(n,P) for n in ('variation','threshold','wind_low','wind_high','wind_limit')]+[(n,C.c_size_t) for n in ('variation_size','threshold_size','wind_low_size','wind_high_size','wind_limit_size')]
class State(C.Structure):
    _fields_=[('players',(U8*64)*2),('hole',U8*256),('auxiliary',U8*17),('order',U8*36)]+[(n,U8) for n in ('difficulty','selected_difficulty','alternate','secondary_mode','ready_flag')]
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def put(a,b):C.memmove(C.addressof(ram.contents)+a-0x4000,bytes(b),len(b))
keep=[]
def span(a,n):
    assert 0x200000<=a<=a+n<=0x400000,(hex(a),n)
    b=(U8*n).from_buffer_copy(data(a,n));keep.append(b);return C.addressof(b)
geometries=(Geometry*54)()
for course in range(3):
    group=0x21bb4d+load(0x21bb4d+2*course,2)
    mapgroup=load(0x36000c,4);mapbase=load(0x360000,4)
    for hole in range(18):
        g=geometries[18*course+hole];address=group+load(group+2*hole,2)
        g.descriptor[:]=data(address,18);g.pins=span(address+load(address+14,2),64);g.pins_size=64
        g.map_index=(load(mapgroup+4*course,2)+18*hole)&65535
        g.aux_index=(load(mapgroup+4*course+12,2)+18*hole)&65535
        record=mapbase+g.map_index;delta=load(record+4,2);delta=delta if delta<32768 else delta-65536
        g.dimensions[:]=data(load(record,4)+delta,2)
        g.descriptor_identity=address;g.path_identity=address+load(address+16,2);g.aux_identity=load(mapbase+g.aux_index+14,4)
hazard_size=0
while load(0x204d81+hazard_size,2)!=65535:
    hazard_size+=10;assert hazard_size<1024
hazard_size+=2
resources=Resources(C.addressof(geometries),54,span(0x204d81,hazard_size),hazard_size,span(0x21d67b,4100),4100,
    span(0x21afc9,22),span(0x21afdf,8),span(0x2050b9,16),span(0x2050c9,16),span(0x2050a7,18),22,8,16,16,18)
window=(U8*256).from_buffer_copy(data(load(0x260010,4)-128,256))
def read_state():
    s=State()
    for bank in range(2):s.players[bank][:]=data(0x4149+64*bank,64)
    s.hole[:]=data(0x4200,256);s.auxiliary[:]=data(0x4400,17);s.order[:]=data(0x441d,36)
    for n,a in (('difficulty',0x4146),('selected_difficulty',0x4147),('alternate',0x41ec),('secondary_mode',0x41ed),('ready_flag',0x41e3)):setattr(s,n,load(a,1))
    return s

def serialize(s,cursor,b):
    for bank in range(2):b[0x149+64*bank:0x189+64*bank]=bytes(s.players[bank])
    b[0x200:0x300]=bytes(s.hole);b[0x400:0x411]=bytes(s.auxiliary);b[0x41d:0x441]=bytes(s.order)
    for n,a in (('difficulty',0x146),('selected_difficulty',0x147),('alternate',0x1ec),('secondary_mode',0x1ed),('ready_flag',0x1e3)):b[a]=getattr(s,n)
    b[14]=cursor.value
