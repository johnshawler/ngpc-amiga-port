#!/usr/bin/env python3
"""Exact synthetic pixels/Chip bitmap verification; not game or speed proof."""
import argparse,json,struct
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('--scene',action='store_true');p.add_argument('--capture',required=True,type=Path);p.add_argument('--chip',required=True,type=Path);p.add_argument('--log',required=True,type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
log=a.log.read_text()
for setting in ['cpu_model=68020','chipmem_size=4','bogomem_size=0','chipset=aga','ntsc=false','Model: "A1200OG".']:
 assert setting in log,setting
assert all(x=='fastmem_size=0' for x in log.splitlines() if x.startswith('fastmem_size=')), 'Fast RAM configured'
ram=a.chip.read_bytes();assert len(ram)==2097152
marker=ram.find(b'NGPDISP1');assert marker>=0 and ram.find(b'NGPDISP1',marker+1)<0
bitmap,copper,clen,bsize,allocation,ready=struct.unpack_from('>6I',ram,marker+8)
assert allocation==marker and bitmap==marker+64 and bsize==81920 and ready==0x52454144
assert copper==bitmap+81920 and copper+clen<marker+100000<=len(ram)
tiles=bytes(((i*37)^(i>>3))&255 for i in range(8192))
pixels=bytearray(320*256)
for ty in range(24):
 for tx in range(32):
  tile=(tx+ty*32)&511;palette=(tx+ty)&15
  for y in range(8):
   source_y=7-y if ty&1 else y
   word=int.from_bytes(tiles[tile*16+source_y*2:tile*16+source_y*2+2],'little')
   for x in range(8):
    source_x=7-x if tx&1 else x
    texel=(word>>(14-source_x*2))&3
    if texel:pixels[(32+ty*8+y)*320+32+tx*8+x]=palette*16+texel*5
if a.scene:
 from scene_expected import expected_pixels
 pixels=expected_pixels()
planes=bytearray(81920)
for y in range(256):
 for x in range(320):
  pen=pixels[y*320+x]
  for bit in range(8):
   if pen&(1<<bit):planes[bit*10240+y*40+x//8]|=128>>(x%8)
assert ram[bitmap:bitmap+81920]==planes,'Chip bitplanes differ'
# Interpret actual palette MOVE pairs to verify both AGA nibble banks.
colors=[[0,0] for _ in range(256)];bank=low=0;controls={}
for pos in range(copper,copper+clen,4):
 reg,val=struct.unpack_from('>HH',ram,pos)
 if reg==0xffff:assert val==0xfffe;break
 controls[reg]=val
 if reg==0x106:bank=(val>>13)&7;low=(val>>9)&1
 elif 0x180<=reg<=0x1be:colors[bank*32+(reg-0x180)//2][low]=val
assert controls[0x100]==0x210 and controls[0x1e4]==0x2100
rgb=[]
for pen,(hi,lo) in enumerate(colors):
 expected=((pen&15)<<8)|(((pen>>4)&15)<<4)|((pen^(pen>>4))&15)
 assert hi==lo==expected
 rgb.append(((pen&15)*17,((pen>>4)&15)*17,((pen^(pen>>4))&15)*17))
im=Image.open(a.capture).convert('RGB')
# This PUAE profile outputs horizontally doubled lowres pixels with a border.
# Infer translation from the synthetic nonzero bounding box, then verify ALL
# pixels including the viewport and outside border, never crop away failures.
expected=Image.new('RGB',(640,256));expected.putdata([rgb[v] for row in range(256) for v in pixels[row*320:(row+1)*320] for _ in range(2)])
box=im.getbbox();ebox=expected.getbbox();assert box and ebox
origin=(box[0]-ebox[0],box[1]-ebox[1]);assert origin[0]>=0 and origin[1]>=0
canvas=Image.new('RGB',im.size);canvas.paste(expected,origin)
assert canvas.tobytes()==im.tobytes(),'Actual AGA capture pixels differ'
r={'result':'PASS','model':'PAL A1200 68020 AGA','chip_bytes':len(ram),'fast_bytes':0,'allocated_chip_bytes':100000,'bitmap_bytes':81920,'copper_bytes':clen,'capture_size':list(im.size),'viewport_origin':list(origin),'checks':['all 81920 Chip bitplane bytes','all 256 AGA palette entries and both nibble writes','every captured RGB pixel including black border','Chip allocation and completed fixture marker'],'limitations':['synthetic static scene fixture only' if a.scene else 'synthetic static tile fixture only','no game/frame timing or audio measured','emulator not physical hardware','fixture takes over hardware until emulator reset; no OS return']}
text=json.dumps(r,indent=2)+'\n';print(text,end='')
if a.report:a.report.write_text(text)
