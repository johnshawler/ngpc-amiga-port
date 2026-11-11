"""Independent destination-pixel/depth oracle for the synthetic AGA scene."""
def expected_pixels(scroll0=251):
    tiles=bytes(((i*37)^(i>>3))&255 for i in range(8192))
    maps=[(i&511)|(((i//7)&15)<<9)|(0x8000 if i&1 else 0)|(0x4000 if i&2 else 0) for i in range(2048)]
    def texel(d,x,y):
        if d&0x8000:x=7-x
        if d&0x4000:y=7-y
        off=(d&511)*16+y*2
        return (int.from_bytes(tiles[off:off+2],'little')>>(14-x*2))&3
    sprites=[];lx=ly=0
    for i in range(64):
        d=i*3|((i%4)<<11)|(0x400 if i&1 else 0)|(0x200 if i&2 else 0)
        x=(i*11)&255;y=(i*7)&255
        if d&0x400:x+=lx
        if d&0x200:y+=ly
        lx,ly=x,y;x+=249;y+=250
        x&=255
        if x>248:x-=256
        y=y-256 if 248<y<256 else y&255
        sprites.append((d,x,y,(i&15)*4))
    pixels=bytearray(320*256)
    for y in range(152):
        for x in range(160):
            pen=253;depth=0
            if 3<=x<157 and 2<=y<149:
                pen=242
                for n in range(2):
                    sx=(x+[scroll0,7][n])&255;sy=(y+[253,19][n])&255
                    d=maps[n*1024+(sy//8)*32+sx//8];t=texel(d,sx%8,sy%8)
                    z=3+2*n
                    if t and z>depth:pen=64+n*64+((d>>9)&15)*4+t;depth=z
                for d,sx,sy,pal in sprites:
                    z=((d>>11)&3)*2
                    if z>depth and sx<=x<sx+8 and sy<=y<sy+8:
                        t=texel(d,x-sx,y-sy)
                        if t:pen=pal+t;depth=z
            pixels[(y+52)*320+x+80]=pen
    return pixels
