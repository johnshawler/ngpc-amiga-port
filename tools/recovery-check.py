#!/usr/bin/env python3
"""Read-only recovery integrity checks; never substitutes for game validation."""
import argparse, configparser, hashlib, json, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE="01c2da88c61f8ec3dcbd2a7c807b72b8e4946d74"
def git(*args):return subprocess.check_output(["git","-C",str(ROOT),*args],text=True)
def verify_rom(path):
 data=Path(path).read_bytes()
 actual={"size":len(data),"sha1":hashlib.sha1(data).hexdigest(),"sha256":hashlib.sha256(data).hexdigest()}
 assert actual["size"]==2097152,actual
 assert actual["sha1"]=="308361297bd28ebb8fb60713ae5e00cee514415c",actual
 assert actual["sha256"]=="92c7b5a3d6d28f8918c030afb95f9e4c13038326b8391d7fb05feac26917051a",actual
 return actual
def check():
 git("merge-base","--is-ancestor",BASE,"HEAD")
 cfg=configparser.ConfigParser();cfg.read(ROOT/"amiga/cfg/a1200-stock-pal.fs-uae")
 expected={"amiga_model":"A1200","chip_memory":"2048","slow_memory":"0","fast_memory":"0","zorro_iii_memory":"0","ntsc_mode":"0"}
 for key,value in expected.items():assert cfg["fs-uae"][key]==value,(key,cfg["fs-uae"][key])
 tracked=git("ls-files").splitlines()
 banned=[p for p in tracked if p.startswith(("private-inputs/","build/","reference/build/")) or Path(p).suffix.lower() in (".rom",".ngc",".ngp",".ngpk",".ngst",".ngsa",".adf")]
 assert not banned,("private/generated inputs tracked",banned)
 pins={}
 for line in git("ls-tree","-r","HEAD").splitlines():
  mode,kind,sha,path=line.split(None,3)
  if mode=="160000":pins[path]=sha
 assert len(pins)==8,pins
 return {"result":"PASS","scope":"source/config integrity only","baseline":BASE,"head":git("rev-parse","HEAD").strip(),"submodule_pins":pins,"stock_config":expected,"game_build_verified":False,"stock_speed_verified":False}
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("--rom");args=p.parse_args()
 result=check()
 if args.rom:result["private_cartridge"]=verify_rom(args.rom)
 print(json.dumps(result,indent=2))
