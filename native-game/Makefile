CC=cc
CROSS_CC=m68k-linux-gnu-gcc
QEMU=qemu-m68k
BUILD=../build/native-game
FLAGS=-std=c99 -O2 -Wall -Wextra -Werror
SOURCES=input_state.c test_input_state.c
.PHONY: test sanitize m68020
$(BUILD):
	mkdir -p $@
test: | $(BUILD)
	$(CC) $(FLAGS) $(SOURCES) -o $(BUILD)/test-input
	$(BUILD)/test-input
sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -o $(BUILD)/test-input-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-input-sanitize
m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c input_state.c -o $(BUILD)/input-68020.o
	test -z "$$(m68k-linux-gnu-nm -u $(BUILD)/input-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static $(SOURCES) -o $(BUILD)/test-input-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-input-68020
