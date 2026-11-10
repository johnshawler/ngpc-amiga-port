PREFIX ?= /opt/amiga
CC      = $(PREFIX)/bin/m68k-amigaos-gcc
CFLAGS  = -O2 -m68020 -noixemul -Wall
OUT     = ../../amiga/hd/hello_aga

$(OUT): main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OUT)
