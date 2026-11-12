# SPDX-License-Identifier: GPL-2.0-or-later
"""Append-only tag27 swing selection mapping; preserves all old eighteen slots."""
from aim_object_assets import _resolve_index

def dense_index(selection):
 base,offset=divmod(selection,64)
 if not (1<=base<=6 and offset in (0,4,8,12,16,20)):raise ValueError('unsupported tag27 swing selection')
 return 3*(base-1)+offset//8 if offset%8==0 else 18+3*(base-1)+(offset-4)//8

def resolve(load,read):
 selections=sorted({4*load(0x21ee93+g,1)+d for g in range(6) for d in (0,4,8,12,16,20)})
 assert load(0x2197d7,2)==60 and len(selections)==36
 r=_resolve_index(load,read,60,selections)
 r['animation_indices']={s:dense_index(s) for s in selections}
 return r
