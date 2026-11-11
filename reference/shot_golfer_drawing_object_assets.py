# SPDX-License-Identifier: GPL-2.0-or-later
"""Resolve the real tag52 resources reached by golfer drawing20a9ce.

The table's values select dispatch entries, not animation offsets. Only its
first two handlers register tag52; the others deliberately skip registration.
"""
from aim_object_assets import _resolve_index

def resolve(load, read):
    assert load(0x219854, 2) == 60
    selections = {load(0x219856, 2)}
    # Immediate LD DE words in the two original registering dispatch handlers.
    for dispatch, immediate in ((0, 0x20aa2b), (4, 0x20aaea)):
        handler = load(0x21b989 + dispatch, 4)
        assert handler == (0x20aa1d if dispatch == 0 else 0x20aadc)
        assert load(immediate - 1, 1) == 0x32
        selections.add(load(immediate, 2))
    assert selections == {16, 20}
    return _resolve_index(load, read, 60, sorted(selections))
