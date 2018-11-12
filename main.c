/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdint.h>
#include <compiler.h>
#include <stdio.h>
#include <task.h>
#include <time.h>
#include <hw/console.h>
#include <hw/pic.h>
#include <hw/pit.h>
#include <x86/x86.h>

static void task_test_routine(void *);

static task_t boot_thread;
static uint32_t boot_stack[128];
static void main2(void *arg);

struct e820 {
    uint64_t base;      // 0x0
    uint64_t len;       // 0x8
    uint32_t type;      // 0x10

    uint8_t  pad[0xc];  // pad out to 0x20 bytes, as returned by the bootloader
} __PACKED;

static void dump_e820(const void *ptr, size_t count) {
    const struct e820 *e820 = ptr;

    for (size_t i = 0; i < count; i++) {
        printf("e820 %zu: base %#llx len %#llx type %lu\n", i, e820[i].base, e820[i].len, e820[i].type);
    }
}

// main C entry point
void _start_c(unsigned int mem, struct e820 *ext_mem_block, size_t ext_mem_count, int in_vesa, void *vesa_ptr) {
    console_init(true);
    x86_init();

    printf("Welcome to 3x86 OS\n");

    printf("Arguments from bootloader:\n\tmem %#x\n\text_mem_block %p ext_mem_count %zu\n\tin_vesa %d vesa_ptr %p\n",
           mem, ext_mem_block, ext_mem_count, in_vesa, vesa_ptr);

    dump_e820(ext_mem_block, ext_mem_count);

    // initialize early hardware
    pic_init();
    pit_init();

    // initialize the tasking subsystem
    task_init();

    // create the boot completion thread
    task_create(&boot_thread, "boot", &main2, NULL, (uintptr_t)boot_stack, sizeof(boot_stack));
    task_start(&boot_thread);

    // kick off the scheduler and become the idle thread
    task_become_idle();
}

void main2(void *arg) {
    printf("top of secondary boot thread\n");

    // TODO: initialize additional drivers and subsystems here

    printf("secondary boot thread exiting\n");
}
