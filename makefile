
CC := i386-elf-gcc
LD := i386-elf-ld
OBJDUMP := i386-elf-objdump
OBJCOPY := i386-elf-objcopy

CFLAGS := -march=i386 -ffreestanding -Os -Iinclude -Wall

BUILD_DIR := build

BOOT_OBJS := bootblock.o
BOOTBLOCK := $(BUILD_DIR)/bootblock

KERNEL_OBJS := \
	start.o \
	main.o \
	console.o \
	string.o \

KERNEL := $(BUILD_DIR)/kernel

IMAGE := $(BUILD_DIR)/image

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

$(IMAGE): $(BOOTBLOCK).bin $(KERNEL).bin
	cat $(BOOTBLOCK).bin $(KERNEL).bin > $@

$(BOOTBLOCK): $(BOOT_OBJS) bootblock.ld
	$(LD) -T bootblock.ld $(BOOT_OBJS) -o $@

$(KERNEL): $(KERNEL_OBJS) kernel.ld
	$(LD) -T kernel.ld $(KERNEL_OBJS) -o $@

$(BUILD_DIR):
	mkdir $@

%.ld:

%.bin: %
	$(OBJCOPY) -Obinary $< $@

%.lst: %
	$(OBJDUMP) -d -M i386 $< > $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

DEPS := $(KERNEL_OBJS:%o=%d)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
