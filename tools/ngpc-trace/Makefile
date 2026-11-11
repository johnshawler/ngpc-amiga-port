# Builds ngpc-trace against the Mednafen NGP core in third_party (sources are not modified:
# the interpreter gets its one-line coverage hook in a patched copy under build/).
CORE    := ../../third_party/ngpc/beetle-ngp-libretro
MED     := $(CORE)/mednafen
EMU     := $(MED)/ngp
COMM    := $(CORE)/libretro-common
BUILD   := build

CC      ?= gcc
CXX     ?= g++
DEFS    := -DINLINE=inline -DLSB_FIRST -DMEDNAFEN_VERSION_NUMERIC=931 -DSTDC_HEADERS -D__STDC_LIMIT_MACROS -D__LIBRETRO__ -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565 -DNO_COMPUTED_GOTO -DNEED_BLIP -DGIT_VERSION=\"ngpc-trace\"
INC     := -I$(CORE) -I$(MED) -I$(MED)/include -I$(MED)/intl -I$(MED)/hw_sound -I$(MED)/hw_cpu \
           -I$(MED)/hw_misc -I$(COMM)/include -I$(EMU)/TLCS-900h
CFLAGS  ?= -O2
CFLAGS  += -w $(DEFS) $(INC)
CXXFLAGS ?= -O2
CXXFLAGS += -w $(DEFS) $(INC)

SRC_C := $(EMU)/biosHLE.c $(EMU)/bios.c $(EMU)/flash.c $(EMU)/dma.c $(EMU)/gfx.c $(EMU)/interrupt.c \
         $(EMU)/mem.c $(EMU)/rom.c $(EMU)/system.c \
         $(EMU)/TLCS-900h/TLCS900h_interpret_dst.c $(EMU)/TLCS-900h/TLCS900h_interpret_reg.c \
         $(EMU)/TLCS-900h/TLCS900h_interpret_single.c $(EMU)/TLCS-900h/TLCS900h_interpret_src.c \
         $(EMU)/TLCS-900h/TLCS900h_registers.c $(MED)/hw_cpu/z80-fuse/z80_ops.c $(MED)/hw_cpu/z80-fuse/z80.c \
         $(EMU)/rtc.c $(EMU)/Z80_interface.c $(MED)/state.c $(MED)/settings.c $(CORE)/libretro.c \
         $(COMM)/streams/file_stream.c $(COMM)/compat/fopen_utf8.c $(COMM)/compat/compat_strl.c \
         $(COMM)/compat/compat_snprintf.c $(COMM)/encodings/encoding_utf.c \
         $(COMM)/string/stdstring.c $(COMM)/file/file_path.c $(COMM)/time/rtime.c $(COMM)/vfs/vfs_implementation.c
SRC_CXX := $(EMU)/sound.cpp $(EMU)/T6W28_Apu.cpp $(MED)/sound/Blip_Buffer.cpp $(MED)/mempatcher.cpp \
           $(MED)/sound/Stereo_Buffer.cpp

OBJ := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRC_C))) $(patsubst %.cpp,$(BUILD)/%.o,$(notdir $(SRC_CXX))) \
       $(BUILD)/TLCS900h_interpret.o $(BUILD)/ngpc-trace.o
vpath %.c $(sort $(dir $(SRC_C)))
vpath %.cpp $(sort $(dir $(SRC_CXX)))

ngpc-trace: $(OBJ)
	$(CXX) -o $@ $(OBJ) -lm

$(BUILD)/TLCS900h_interpret.c: $(EMU)/TLCS-900h/TLCS900h_interpret.c | $(BUILD)
	sed -e 's|^\tbrCode = false;|\tngpc_trace_pc(pc);\n\tbrCode = false;|' \
	    -e '1i void ngpc_trace_pc(unsigned int pc);' $< > $@
	grep -q 'ngpc_trace_pc(pc);' $@

$(BUILD)/TLCS900h_interpret.o: $(BUILD)/TLCS900h_interpret.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/ngpc-trace.o: ngpc-trace.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD) ngpc-trace
