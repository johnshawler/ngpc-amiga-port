#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Complete integrated scheduler through real2098af viewport opening."""
import os,runpy
from pathlib import Path
os.environ['NTM_SHOT_AIM_CONTINUATION']='1'
os.environ['NTM_SHOT_OPEN_CONTINUATION']='1'
runpy.run_path(str(Path(__file__).with_name('compare_shot_application.py')),run_name='__main__')
