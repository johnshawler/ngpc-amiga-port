#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the existing scheduler oracle with the real opt-in aim continuation."""
import os,runpy
from pathlib import Path
os.environ['NTM_SHOT_AIM_CONTINUATION']='1'
runpy.run_path(str(Path(__file__).with_name('compare_shot_application.py')),run_name='__main__')
