
CC := i386-elf-gcc
LD := i386-elf-ld
OBJDUMP := i386-elf-objdump
OBJCOPY := i386-elf-objcopy

CFLAGS := -march=i386 -ffreestanding -Os --std=gnu11 -fbuiltin
CFLAGS += -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Werror=return-type -Wno-nonnull-compare
CFLAGS += -Werror-implicit-function-declaration -Wstrict-prototypes -Wwrite-strings
INCLUDES := -Iinclude

LIBGCC := $(shell $(CC) $(CFLAGS) --print-libgcc-file-name)

BUILD_DIR := build

BOOT_OBJS := bootblock.o
BOOTBLOCK := $(BUILD_DIR)/bootblock

KERNEL_OBJS := \
	start.o \
	main.o \
	exceptions.o \
\
	console.o \
	printf.o \
	stdio.o \
	string.o \
	x86.o

KERNEL := $(BUILD_DIR)/kernel
IMAGE := $(BUILD_DIR)/image

MAKEFLOP := $(BUILD_DIR)/makeflop

BOOT_OBJS := $(addprefix $(BUILD_DIR)/,$(BOOT_OBJS))
KERNEL_OBJS := $(addprefix $(BUILD_DIR)/,$(KERNEL_OBJS))

.PHONY: all
all: $(IMAGE) $(BOOTBLOCK).lst $(KERNEL).lst

.PHONY: clean
clean:
	rm -rf -- $(BUILD_DIR)

.PHONY: qemu
qemu: all
	qemu-system-i386 --monitor stdio --machine isapc --cpu 486 -m 4 -fda $(IMAGE)

.PHONY: format
format:
	astyle -j -A2 --align-pointer=name --indent=spaces=4 --indent-switches --keep-one-line-blocks --pad-header --convert-tabs -r \*.c \*.h

$(IMAGE): $(BOOTBLOCK).bin $(KERNEL).bin $(MAKEFLOP) | $(BUILD_DIR)
	$(MAKEFLOP) $(BOOTBLOCK).bin $(KERNEL).bin $@

$(BOOTBLOCK): $(BOOT_OBJS) bootblock.ld | $(BUILD_DIR)
	$(LD) -T bootblock.ld $(BOOT_OBJS) -o $@

$(KERNEL): $(KERNEL_OBJS) kernel.ld | $(BUILD_DIR)
	$(LD) -T kernel.ld $(KERNEL_OBJS) -o $@ $(LIBGCC)

$(MAKEFLOP): makeflop.c | $(BUILD_DIR)
	cc -O -Wall $< -o $@

$(BUILD_DIR):
	mkdir $@

%.ld:

%.bin: %
	$(OBJCOPY) -Obinary $< $@

%.lst: %
	$(OBJDUMP) -d -M i386 $< > $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) $(INCLUDES) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

DEPS := $(KERNEL_OBJS:%o=%d)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
