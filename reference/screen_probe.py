"""Isolated original routine test harness; fixed local user-supplied cartridge."""
import ctypes as C,sys,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'reference'))
from run import ENV,VIDEO,AUDIO,POLL,INPUT,Game,Variable
core=C.CDLL(str(ROOT/'reference/build/screen-probe/mednafen_ngp_libretro.so'))
directory=str(ROOT/'build/course-screen-check').encode()
@ENV
def env(cmd,data):
 if cmd in (9,31):C.cast(data,C.POINTER(C.c_char_p))[0]=directory;return True
 if cmd==10:return C.cast(data,C.POINTER(C.c_int))[0]==2
 if cmd==15:
  var=C.cast(data,C.POINTER(Variable)).contents
  if var.key==b'ngp_language':var.value=b'english';return True
  return False
 if cmd in (2,17):C.cast(data,C.POINTER(C.c_bool))[0]=False;return True
 return cmd in (8,11,16)
@VIDEO
def vid(*a):pass
@AUDIO
def audio(p,n):return n
@POLL
def poll():pass
@INPUT
def inp(*a):return 0
callbacks=[('environment',env),('video_refresh',vid),('audio_sample_batch',audio),('input_poll',poll),('input_state',inp)]
for n,cb in callbacks:getattr(core,'retro_set_'+n)(cb)
core.retro_init()
rom=(ROOT/'private-inputs/neo-turf-masters.ngc').read_bytes(); buf=C.create_string_buffer(rom)
game=Game(b'game.ngc',C.cast(buf,C.c_void_p),len(rom),None)
core.retro_load_game.argtypes=[C.POINTER(Game)];assert core.retro_load_game(C.byref(game))
for name,n in [('reg',2),('getreg',1),('store',3),('load',2),('step',1)]:
 fn=getattr(core,'retro_reference_life_'+name);fn.argtypes=[C.c_uint]*n;fn.restype=C.c_uint
reg=core.retro_reference_life_reg;get=core.retro_reference_life_getreg;store=core.retro_reference_life_store;load=core.retro_reference_life_load;step=core.retro_reference_life_step

def setup():
 core.retro_reference_life_reset();reg(7,0x6be0);store(0x6be0,0xef00,4)
