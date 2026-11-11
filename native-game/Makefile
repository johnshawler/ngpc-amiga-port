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

.PHONY: course course-sanitize course-m68020
course: | $(BUILD)
	$(CC) $(FLAGS) input_state.c course_choice.c test_course_choice.c -o $(BUILD)/test-course
	$(BUILD)/test-course
course-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer input_state.c course_choice.c test_course_choice.c -o $(BUILD)/test-course-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-course-sanitize
course-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c course_choice.c -o $(BUILD)/course-68020.o
	$(CROSS_CC) $(FLAGS) -m68020 -static input_state.c course_choice.c test_course_choice.c -o $(BUILD)/test-course-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-course-68020

LIFECYCLE=input_state.c course_choice.c task_lifecycle.c test_task_lifecycle.c
.PHONY: lifecycle lifecycle-sanitize lifecycle-m68020
lifecycle: | $(BUILD)
	$(CC) $(FLAGS) $(LIFECYCLE) -o $(BUILD)/test-lifecycle
	$(BUILD)/test-lifecycle
lifecycle-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(LIFECYCLE) -o $(BUILD)/test-lifecycle-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-lifecycle-sanitize
lifecycle-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c task_lifecycle.c -o $(BUILD)/lifecycle-68020.o
	$(CROSS_CC) $(FLAGS) -m68020 -static $(LIFECYCLE) -o $(BUILD)/test-lifecycle-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-lifecycle-68020

ALLOCATION=task_allocation.c test_task_allocation.c
.PHONY: allocation allocation-sanitize allocation-m68020
allocation: | $(BUILD)
	$(CC) $(FLAGS) $(ALLOCATION) -o $(BUILD)/test-allocation
	$(BUILD)/test-allocation
allocation-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(ALLOCATION) -o $(BUILD)/test-allocation-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-allocation-sanitize
allocation-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c task_allocation.c -o $(BUILD)/allocation-68020.o
	test -z "$$(m68k-linux-gnu-nm -u $(BUILD)/allocation-68020.o)"
	$(CROSS_CC) $(FLAGS) -m68020 -static $(ALLOCATION) -o $(BUILD)/test-allocation-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-allocation-68020

PARENT=input_state.c course_choice.c task_lifecycle.c task_allocation.c random_byte.c viewport.c game_services.c course_screen.c course_parent.c test_course_parent.c
.PHONY: parent parent-sanitize parent-m68020
parent: | $(BUILD)
	$(CC) $(FLAGS) $(PARENT) -o $(BUILD)/test-parent
	$(BUILD)/test-parent
parent-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(PARENT) -o $(BUILD)/test-parent-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-parent-sanitize
parent-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c course_parent.c -o $(BUILD)/parent-68020.o
	$(CROSS_CC) $(FLAGS) -m68020 -static $(PARENT) -o $(BUILD)/test-parent-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-parent-68020

CONTINUATIONS=input_state.c course_choice.c task_lifecycle.c task_allocation.c random_byte.c viewport.c game_services.c course_screen.c course_parent.c test_parent_continuations.c
.PHONY: continuations continuations-sanitize continuations-m68020
continuations: | $(BUILD)
	$(CC) $(FLAGS) $(CONTINUATIONS) -o $(BUILD)/test-continuations
	$(BUILD)/test-continuations
continuations-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(CONTINUATIONS) -o $(BUILD)/test-continuations-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-continuations-sanitize
continuations-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(CONTINUATIONS) -o $(BUILD)/test-continuations-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-continuations-68020

SERVICES=input_state.c course_choice.c task_lifecycle.c task_allocation.c random_byte.c course_parent.c course_screen.c viewport.c game_services.c render_bridge.c ../native-renderer/scene_renderer.c ../native-renderer/tile_renderer.c test_game_services.c
.PHONY: services services-sanitize services-m68020
services: | $(BUILD)
	$(CC) $(FLAGS) $(SERVICES) -o $(BUILD)/test-services
	$(BUILD)/test-services
services-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(SERVICES) -o $(BUILD)/test-services-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-services-sanitize
services-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(SERVICES) -o $(BUILD)/test-services-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-services-68020

SCREEN=course_screen.c test_course_screen.c
.PHONY: screen screen-sanitize screen-m68020
screen: | $(BUILD)
	$(CC) $(FLAGS) $(SCREEN) -o $(BUILD)/test-screen
	$(BUILD)/test-screen
screen-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(SCREEN) -o $(BUILD)/test-screen-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-screen-sanitize
screen-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(SCREEN) -o $(BUILD)/test-screen-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-screen-68020

INITIALIZATION=course_initialization.c course_palette.c course_screen.c game_services.c test_course_initialization.c
DRIVER=input_state.c course_choice.c task_lifecycle.c task_allocation.c random_byte.c viewport.c game_services.c course_parent.c course_screen.c course_driver.c test_course_driver.c
.PHONY: initialization initialization-sanitize initialization-m68020 driver driver-sanitize driver-m68020
initialization: | $(BUILD)
	$(CC) $(FLAGS) $(INITIALIZATION) -o $(BUILD)/test-initialization
	$(BUILD)/test-initialization
initialization-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(INITIALIZATION) -o $(BUILD)/test-initialization-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-initialization-sanitize
initialization-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(INITIALIZATION) -o $(BUILD)/test-initialization-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-initialization-68020
driver: | $(BUILD)
	$(CC) $(FLAGS) $(DRIVER) -o $(BUILD)/test-driver
	$(BUILD)/test-driver
driver-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(DRIVER) -o $(BUILD)/test-driver-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-driver-sanitize
driver-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(DRIVER) -o $(BUILD)/test-driver-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-driver-68020

APPLICATION=$(filter-out test_course_driver.c,$(DRIVER)) course_palette.c course_initialization.c course_application.c course_cursor.c ../native-renderer/scene_renderer.c ../native-renderer/tile_renderer.c test_course_application.c
.PHONY: application application-sanitize application-m68020
application: | $(BUILD)
	$(CC) $(FLAGS) $(APPLICATION) -o $(BUILD)/test-application
	$(BUILD)/test-application
application-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(APPLICATION) -o $(BUILD)/test-application-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-application-sanitize
application-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(APPLICATION) -o $(BUILD)/test-application-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-application-68020

CURSOR=course_cursor.c game_services.c test_course_cursor.c
.PHONY: cursor cursor-sanitize cursor-m68020
cursor: | $(BUILD)
	$(CC) $(FLAGS) $(CURSOR) -o $(BUILD)/test-cursor
	$(BUILD)/test-cursor
cursor-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(CURSOR) -o $(BUILD)/test-cursor-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-cursor-sanitize
cursor-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(CURSOR) -o $(BUILD)/test-cursor-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-cursor-68020

INTRO=input_state.c course_intro.c course_palette.c course_screen.c game_services.c task_allocation.c task_lifecycle.c course_choice.c test_course_intro.c
.PHONY: intro intro-sanitize intro-m68020
intro: | $(BUILD)
	$(CC) $(FLAGS) $(INTRO) -o $(BUILD)/test-intro
	$(BUILD)/test-intro
intro-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(INTRO) -o $(BUILD)/test-intro-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-intro-sanitize
intro-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c course_intro.c -o $(BUILD)/intro-68020.o
	$(CROSS_CC) $(FLAGS) -m68020 -static $(INTRO) -o $(BUILD)/test-intro-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-intro-68020

TITLE_CURSOR=course_cursor.c game_services.c test_course_title_cursor.c
.PHONY: title-cursor title-cursor-sanitize title-cursor-m68020
title-cursor: | $(BUILD)
	$(CC) $(FLAGS) $(TITLE_CURSOR) -o $(BUILD)/test-title-cursor
	$(BUILD)/test-title-cursor
title-cursor-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(TITLE_CURSOR) -o $(BUILD)/test-title-cursor-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-title-cursor-sanitize
title-cursor-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(TITLE_CURSOR) -o $(BUILD)/test-title-cursor-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-title-cursor-68020

TITLE = test_course_title.c course_title.c course_screen.c game_services.c task_allocation.c task_lifecycle.c course_choice.c input_state.c
.PHONY: title title-sanitize title-m68020
title: | $(BUILD)
	$(CC) $(FLAGS) $(TITLE) -o $(BUILD)/test-title
	$(BUILD)/test-title
title-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(TITLE) -o $(BUILD)/test-title-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-title-sanitize
title-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(TITLE) -o $(BUILD)/test-title-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-title-68020

ROUND_PREPARE=round_prepare.c random_byte.c test_round_prepare.c
.PHONY: round-prepare round-prepare-sanitize round-prepare-m68020
round-prepare: | $(BUILD)
	$(CC) $(FLAGS) $(ROUND_PREPARE) -o $(BUILD)/test-round-prepare
	$(BUILD)/test-round-prepare
round-prepare-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(ROUND_PREPARE) -o $(BUILD)/test-round-prepare-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-round-prepare-sanitize
round-prepare-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -msoft-float -ffreestanding -fno-builtin -c round_prepare.c -o $(BUILD)/round-prepare-68020.o
	$(CROSS_CC) $(FLAGS) -m68020 -static $(ROUND_PREPARE) -o $(BUILD)/test-round-prepare-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-round-prepare-68020

ROUND_RECORD=round_record.c course_title.c course_screen.c course_palette.c game_services.c task_allocation.c task_lifecycle.c course_choice.c input_state.c test_round_record.c
.PHONY: round-record round-record-sanitize round-record-m68020
round-record: | $(BUILD)
	$(CC) $(FLAGS) $(ROUND_RECORD) -o $(BUILD)/test-round-record
	$(BUILD)/test-round-record
round-record-sanitize: | $(BUILD)
	$(CC) $(FLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(ROUND_RECORD) -o $(BUILD)/test-round-record-sanitize
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/test-round-record-sanitize
round-record-m68020: | $(BUILD)
	$(CROSS_CC) $(FLAGS) -m68020 -static $(ROUND_RECORD) -o $(BUILD)/test-round-record-68020
	$(QEMU) -cpu m68020 $(BUILD)/test-round-record-68020
