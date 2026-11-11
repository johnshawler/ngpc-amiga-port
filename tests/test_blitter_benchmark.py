"""Discover the standalone no-ROM Blitter verifier's corruption tests in CI."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'native-renderer/amiga'))
from blitter_test import VerifierTests
