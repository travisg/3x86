
CC := i386-elf-gcc
LD := i386-elf-ld
OBJDUMP := i386-elf-objdump
OBJCOPY := i386-elf-objcopy

CFLAGS := -march=i386 -ffreestanding -Os -Iinclude -Wall

BOOT_OBJS := bootblock.o
BOOTBLOCK := bootblock

KERNEL_OBJS := \
	start.o \
	main.o \
	console.o \
	string.o \

KERNEL := kernel

IMAGE := image

.PHONY: all
all: $(IMAGE) $(BOOTBLOCK).lst $(KERNEL).lst

.PHONY: clean
clean:
	rm -f -- $(BOOTBLOCK) $(BOOTBLOCK).lst $(BOOTBLOCK).bin
	rm -f -- $(KERNEL) $(KERNEL).lst $(KERNEL).bin
	rm -f -- $(IMAGE)
	rm -f -- *.o
	rm -f -- *.d

.PHONY: qemu
qemu: all
	qemu-system-i386 --monitor stdio --machine isapc --cpu 486 -m 4 -fda $(IMAGE)

$(IMAGE): $(BOOTBLOCK).bin $(KERNEL).bin
	cat $(BOOTBLOCK).bin kernel.bin > $@

$(BOOTBLOCK): $(BOOT_OBJS) bootblock.ld
	$(LD) -T bootblock.ld $(BOOT_OBJS) -o $@

$(KERNEL): $(KERNEL_OBJS) kernel.ld
	$(LD) -T kernel.ld $(KERNEL_OBJS) -o $@

bootblock.ld:

kernel.ld:

%.bin: %
	$(OBJCOPY) -Obinary $< $@

%.lst: %
	$(OBJDUMP) -d -M i386 $< > $@

%.o: %.S
	$(CC) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

DEPS := $(KERNEL_OBJS:%o=%d)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif
