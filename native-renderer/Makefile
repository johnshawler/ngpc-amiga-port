# Host validation by default. Cross validation needs existing tools on PATH.
CC = cc
CROSS_CC = m68k-linux-gnu-gcc
CROSS_NM = m68k-linux-gnu-nm
QEMU = qemu-m68k
QEMU_FLAGS = -cpu m68020
BUILD = ../build/native-renderer
FLAGS = -std=c99 -Wall -Wextra -Werror -O2
SOURCES = tile_renderer.c test_renderer.c
.PHONY: test sanitize m68020
$(BUILD):
	mkdir -p $@
test: | $(BUILD)
	$(CC) $(FLAGS) $(SOURCES) -o $(BUILD)/test
	$(BUILD)/test
sanitize: | $(BUILD)
	$(CC) $(FLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -o $(BUILD)/test-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-sanitize
m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c tile_renderer.c -o $(BUILD)/tile_renderer-68020.o
	test -z "$$($(CROSS_NM) -u $(BUILD)/tile_renderer-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static $(SOURCES) -o $(BUILD)/test-68020
	$(QEMU) $(QEMU_FLAGS) $(BUILD)/test-68020

.PHONY: scene scene-sanitize scene-m68020
scene: | $(BUILD)
	$(CC) $(FLAGS) scene_renderer.c test_scene.c -o $(BUILD)/test-scene
	$(BUILD)/test-scene
scene-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer scene_renderer.c test_scene.c -o $(BUILD)/test-scene-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-scene-sanitize
scene-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c scene_renderer.c -o $(BUILD)/scene-renderer-68020.o
	test -z "$$($(CROSS_NM) -u $(BUILD)/scene-renderer-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static scene_renderer.c test_scene.c -o $(BUILD)/test-scene-68020
	$(QEMU) $(QEMU_FLAGS) $(BUILD)/test-scene-68020

.PHONY: planar planar-sanitize planar-m68020
planar: | $(BUILD)
	$(CC) $(FLAGS) scene_renderer.c test_planar.c -o $(BUILD)/test-planar
	$(BUILD)/test-planar
planar-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer scene_renderer.c test_planar.c -o $(BUILD)/test-planar-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-planar-sanitize
planar-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c scene_renderer.c -o $(BUILD)/scene-renderer-68020.o
	test -z "$$($(CROSS_NM) -u $(BUILD)/scene-renderer-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static scene_renderer.c test_planar.c -o $(BUILD)/test-planar-68020
	$(QEMU) $(QEMU_FLAGS) $(BUILD)/test-planar-68020

.PHONY: direct direct-sanitize direct-m68020
direct: | $(BUILD)
	$(CC) $(FLAGS) scene_renderer.c test_direct.c -o $(BUILD)/test-direct
	$(BUILD)/test-direct
direct-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -g -fsanitize=address,undefined -fno-omit-frame-pointer scene_renderer.c test_direct.c -o $(BUILD)/test-direct-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-direct-sanitize
direct-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c scene_renderer.c -o $(BUILD)/scene-renderer-68020.o
	test -z "$$($(CROSS_NM) -u $(BUILD)/scene-renderer-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static scene_renderer.c test_direct.c -o $(BUILD)/test-direct-68020
	$(QEMU) $(QEMU_FLAGS) $(BUILD)/test-direct-68020
