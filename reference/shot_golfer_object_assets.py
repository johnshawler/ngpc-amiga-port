# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve exact tag51 initializer and distance-selected pin resources."""
from aim_object_assets import _resolve_index

def resolve(load, read):
    selections = {load(0x219851, 2)}
    at = 0x21b955
    while True:
        selections.add(load(at + 2, 2))
        if load(at, 2) == 65535:
            break
        at += 4
        assert at < 0x21b979
    assert load(0x21984f, 2) == 60
    return _resolve_index(load, read, 60, sorted(selections))
