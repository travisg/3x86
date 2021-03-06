
USE_LTO ?= 1

CC := i386-elf-gcc
LD := i386-elf-ld
OBJDUMP := i386-elf-objdump
OBJCOPY := i386-elf-objcopy
SIZE := i386-elf-size

CFLAGS := -march=i386 -ffreestanding -Os --std=gnu11 -fbuiltin -nostdlib
CFLAGS += -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Werror=return-type -Wno-nonnull-compare
CFLAGS += -Werror-implicit-function-declaration -Wstrict-prototypes -Wwrite-strings
ifeq ($(USE_LTO),1)
CFLAGS += -flto
endif
INCLUDES := -Iinclude

# a particular usb floppy drive that I have for testing on real hardware
FLOPPY_DEV ?= /dev/disk/by-id/usb-TEACV0.0_TEACV0.0

LIBGCC := $(shell $(CC) $(CFLAGS) --print-libgcc-file-name)

BUILD_DIR := build

BOOT_OBJS := bootblock.o
BOOTBLOCK := $(BUILD_DIR)/bootblock

KERNEL_OBJS := \
	console.o \
	ctype.o \
	debug.o \
	heap.o \
	main.o \
	miniheap.o \
	printf.o \
	start.o \
	stdio.o \
	string.o \
	task.o \
\
	hw/keyboard.o \
	hw/pic.o \
	hw/pit.o \
	hw/vga.o \
\
	x86/exceptions.o \
	x86/task.o \
	x86/task_asm.o \
	x86/tss.o \
	x86/x86.o

KERNEL := $(BUILD_DIR)/kernel
IMAGE := $(BUILD_DIR)/image
IMAGE_PADDED := $(BUILD_DIR)/image.padded

MAKEFLOP := $(BUILD_DIR)/makeflop

BOOT_OBJS := $(addprefix $(BUILD_DIR)/,$(BOOT_OBJS))
KERNEL_OBJS := $(addprefix $(BUILD_DIR)/,$(KERNEL_OBJS))

# makes sure the target dir exists
MKDIR = mkdir -p $(dir $@)

.PHONY: all
all: $(IMAGE) $(IMAGE_PADDED) $(BOOTBLOCK).lst $(KERNEL).lst

.PHONY: clean
clean:
	rm -rf -- $(BUILD_DIR)

.PHONY: qemu
qemu: all
	qemu-system-i386 --monitor stdio --machine pc --cpu 486 -m 4 -drive if=floppy,format=raw,file=$(IMAGE_PADDED) -no-shutdown

.PHONY: format
format:
	astyle -j -A2 --align-pointer=name --indent=spaces=4 --indent-switches --keep-one-line-blocks --pad-header --convert-tabs -r \*.c \*.h

.PHONY: disk
disk: $(IMAGE)
	@if [ -b $(FLOPPY_DEV) ]; then \
		echo writing to $(FLOPPY_DEV); \
		sudo dd if=$(IMAGE) of=$(FLOPPY_DEV) bs=512 conv=fdatasync; \
	fi

$(IMAGE): $(BOOTBLOCK).bin $(KERNEL).bin $(MAKEFLOP) makefile
	@$(MKDIR)
	$(MAKEFLOP) -p 512 $(BOOTBLOCK).bin $(KERNEL).bin $@

$(IMAGE_PADDED): $(BOOTBLOCK).bin $(KERNEL).bin $(MAKEFLOP) makefile
	@$(MKDIR)
	$(MAKEFLOP) -p $$((80 * 18 * 2 * 512)) $(BOOTBLOCK).bin $(KERNEL).bin $@

$(BOOTBLOCK): $(BOOT_OBJS) bootblock.ld makefile
	@$(MKDIR)
	$(CC) $(CFLAGS) -T bootblock.ld $(BOOT_OBJS) -o $@

$(KERNEL): $(KERNEL_OBJS) kernel.ld makefile
	@$(MKDIR)
	$(CC) $(CFLAGS) -T kernel.ld $(KERNEL_OBJS) -o $@ $(LIBGCC)
	$(SIZE) $@

$(MAKEFLOP): makeflop.c makefile
	@$(MKDIR)
	cc -O -Wall $< -o $@

%.ld:

%.bin: % makefile
	@$(MKDIR)
	$(OBJCOPY) -Obinary $< $@

%.lst: % makefile
	@$(MKDIR)
	$(OBJDUMP) -d -M i386 $< > $@

$(BUILD_DIR)/%.o: %.S makefile
	@$(MKDIR)
	$(CC) $(INCLUDES) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

$(BUILD_DIR)/%.o: %.c makefile
	@$(MKDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

DEPS := $(KERNEL_OBJS:%o=%d)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
