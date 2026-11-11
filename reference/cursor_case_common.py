#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Private original comparison; no cartridge bytes or captured assets embedded.
from screen_probe import *
import random,json
R=random.Random(0x453b)
def data(a,n):return bytes(load(a+i,1) for i in range(n))
def put(a,b):
 for j,v in enumerate(b):store(a+j,v,1)
def execute(pc):
 reg(7,0x6be0);store(0x6be0,0xef00,4);reg(8,pc)
 for _ in range(100000):
  if get(8)==0xef00:return
  step(0)
 raise RuntimeError(hex(get(8)))
def init():
 setup();reg(6,0x7000);store(0x40fa,16,1);store(0x87e2,0,1)
 for j in range(8):store(0x66bf+2*j,0xff00+j*4,2);store(0x5782+j,255,1)
 execute(0x20921e)
anim=load(load(load(0x260024,4)+16,4)&0xbfffff,4)&0xffffff
shapes=load(load(0x260028,4)+16,4)

def word(b,o):return int.from_bytes(b[o:o+2],'little')
def signed(x):return (x^128)-128
class Model:
 def __init__(self):self.b=bytearray(ram[:16384]);self.tiles=bytearray(data(0xa000,8192));self.sprites=data(0x8800,256);self.pal=data(0x8c00,64)
 def r(self,a,n=1):return int.from_bytes(self.b[a-0x4000:a-0x4000+n],'little')
 def w(self,a,v,n=1):self.b[a-0x4000:a-0x4000+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
 def tail(self):
  p=0x453b
  anim=load((load(load(0x260024,4)+16,4)&0xbfffff)+self.r(p+10,2),4)&0xffffff
  # single active cursor list, no removal, second byte matches list priority
  if self.r(0x4d7d) and self.r(p+16)&16:
   assert self.r(p+30)&1==0
   self.w(p+2,65535,2);self.w(0x4d7b,0,2);self.w(0x4d7d,0);self.w(0x4d3b,0,2)
  self.w(0x4d7e,0);self.w(0x4d7f,0)
  for a in range(0x61a6,0x61a9):self.w(a,0)
  if not self.r(0x4d7d):
   dest=0x643b if self.r(0x61ba)&1 else 0x61bb
   for offset in (0,0x140):
    for j in range(self.r(0x66bc)):
     self.w(dest+offset+4*j,0,4);self.w(dest+offset+256+j,0)
   self.w(0x61ba,self.r(0x61ba)^1);return
  oldflags=self.r(p+16);status=self.r(p+30)&0x7f;changed=bool(self.r(0x400d)&0x20)
  if oldflags&0x80:
   self.w(p+16,oldflags&0x7f);status&=~0x60;changed=True;self.w(p+12,0);self.w(p+13,load(anim,1)+1)
  self.w(p+16,self.r(p+16)|2)
  if not(oldflags&0x40):
   self.w(p+16,self.r(p+16)&~2);t=(self.r(p+13)-1)&255;self.w(p+13,t)
   if t==0:
    changed=True;f=load(anim+self.r(p+12)+1,1)
    if f&0x80:
     assert not f&0x40;status|=0x20;self.w(p+12,252)
    self.w(p+12,self.r(p+12)+4);self.w(p+13,load(anim+self.r(p+12),1));status|=0x80
  self.w(p+16,self.r(p+16)|1);hidden=bool(oldflags&0x20)
  if not hidden:self.w(p+16,self.r(p+16)&~1)
  self.w(p+30,status)
  for a in range(p+20,p+28,2):self.w(a,65535,2)
  component=anim+load(anim+self.r(p+12)+2,2);shape=shapes+(load(component,2)&32767)
  base=self.r(p+18,2);priority=self.r(p+15)&3
  bx=(self.r(p+4,2)-self.r(0x4d80,2)+(-signed(load(component+2,1)) if oldflags&8 else signed(load(component+2,1))))&65535
  by=(self.r(p+6,2)-self.r(0x4d82,2)+(-signed(load(component+3,1)) if oldflags&4 else signed(load(component+3,1))))&65535
  bank=bool(self.r(0x61ba)&1);dest=0x643b if bank else 0x61bb;pal=dest+256;cnt=0;i=0
  while True:
   a=load(shape+2*i,1);xy=load(shape+2*i+1,1)
   x=(bx+(-8*(xy&15)-8 if oldflags&8 else 8*(xy&15)))&65535
   y=(by+(-8*(xy>>4)-8 if oldflags&4 else 8*(xy>>4)))&65535
   if not hidden and ((x+7)&65535)<=166 and ((y+7)&65535)<=158 and cnt<self.r(0x66bc):
    attr=((base+i)|(priority<<11)|((a&0xf0)<<9))&65535
    attr^=(oldflags&12)<<12
    if self.r(p+14)&128:attr^=0x2000
    self.w(dest+4*cnt,attr,2);self.w(dest+4*cnt+2,x);self.w(dest+4*cnt+3,y);self.w(pal+cnt,(a+self.r(p+14))&15);cnt+=1
   i+=1
   if a&128:break
  q=shape+2*i
  if changed and self.r(0x6182)<64:
   off=self.r(0x6180,2);self.w(0x6000+off,i);self.w(0x6001+off,base);self.w(0x6002+off,q|((base>>8)<<24),4);self.w(0x6180,off+6,2);self.w(0x6182,self.r(0x6182)+1)
  self.w(0x5bcd,base+i,2);self.w(0x5bcb,(base+i)|(self.r(p+15)<<11),2);self.w(0x61a6,cnt)
  for offset,n in ((0,cnt),(0x140,0)):
   for j in range(n,self.r(0x66bc)):
    self.w(dest+offset+4*j,0,4);self.w(pal+offset+j,0)
  self.w(0x400d,self.r(0x400d)&~32);self.w(0x61ba,self.r(0x61ba)^1)
 def head(self):
  for i in range(self.r(0x6182)):
   q=0x6000+6*i;n=self.r(q)&127;tile=self.r(q+1)|(self.r(q+5)<<8);src=self.r(q+2,4)&0xffffff;ts=load(load(src,4),4)
   for j in range(n):self.tiles[16*(tile+j):16*(tile+j+1)]=data(ts+16*load(src+4+2*j,2),16)
  self.w(0x6182,0);self.w(0x6180,0,2)
  dest=0x61bb if self.r(0x61ba)&1 else 0x643b
  for offset in (0,0x140):
   for j in range(64):self.w(dest+offset+4*j+1,self.r(dest+offset+4*j+1)^((self.r(dest+offset+256+j)&48)>>1))
  st=self.r(0x66bb);n=self.r(0x66bc);s=bytearray(self.sprites);s[4*st:4*(st+n)]=self.b[dest-0x4000:dest-0x4000+4*n];self.sprites=bytes(s)
  pal=bytearray(self.pal);pal[st:st+n]=bytes(x&15 for x in self.b[dest+256-0x4000:dest+256-0x4000+n]);self.pal=bytes(pal)
  self.w(0x619e,0x61aa,4);self.w(0x61a2,dest+0x140,4);self.w(0x619d,1);self.w(0x61ba,(self.r(0x61ba)&~4)^2)
 def compare(self,label):
  b=bytes(ram[:16384]);diff=[(hex(0x4000+i),x,y) for i,(x,y) in enumerate(zip(b,self.b)) if x!=y and not 0x6b00<=i+0x4000<0x6be4]
  assert not diff,(label,diff[:12])
  assert data(0xa000,8192)==bytes(self.tiles),(label,'tiles')
  assert data(0x8800,256)==self.sprites,(label,'sprites')
  assert data(0x8c00,64)==self.pal,(label,'pal')
def reset_model(m):
 for a in (0x66bb,0x61ba,0x61b4,0x61b5,0x61b7,0x61b8,0x4d80,0x4d82,0x4d7d,0x4d7e,0x6182,0x619d,0x61a9,0x61a6,0x61a7,0x61a8):m.w(a,0)
 m.w(0x4d7b,0,2);m.w(0x6180,0,2);m.w(0x66bc,64);m.w(0x61aa,80)
 for i in range(32):m.w(0x453b+64*i,i);m.w(0x453d+64*i,65535,2);m.w(0x4d3b+2*i,0,2)
 for i in range(256):m.w(0x61bb+i,0);m.w(0x62fb+i,0)
 m.w(0x5bcb,0x974c54f9,4);m.w(0x61ac,0x4b4c5bf9,4)
