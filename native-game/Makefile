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

PARENT=input_state.c course_choice.c task_lifecycle.c task_allocation.c course_parent.c test_course_parent.c
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
