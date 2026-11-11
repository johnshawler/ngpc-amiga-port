# SPDX-License-Identifier: GPL-2.0-or-later
"""Borrowed tag27 lifecycle1 sprite spans and native dense animation indices."""
from aim_object_assets import _resolve_index

def dense_index(selection):
    if not (64 <= selection <= 400 and selection % 64 <= 16 and selection % 8 == 0):
        raise ValueError('unsupported tag27 lifecycle1 selection')
    return 3 * (selection // 64 - 1) + (selection % 64) // 8

def resolve(load, read):
    selections = sorted({4 * load(0x21ee93 + golfer, 1) + delta
                         for golfer in range(6) for delta in (0, 8, 16)})
    assert load(0x2197d7, 2) == 60
    assert len(selections) == 18
    result = _resolve_index(load, read, 60, selections)
    result['animation_indices'] = {selection: dense_index(selection) for selection in selections}
    return result
