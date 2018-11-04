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
#include <x86/x86.h>

#include <stdio.h>
#include <string.h>
#include <task.h>

static struct x86_tss kernel_tss;

static uint32_t tss_slot_bitmap = 0;
_Static_assert(sizeof(tss_slot_bitmap) * 8 >= TASK_TSS_SELECTOR_COUNT, "");

void x86_tss_init(void) {
    // initialize the kernel tss and switch to it

    printf("kernel tss %p\n", &kernel_tss);

    // punch in the address of the kernel_tss
    struct x86_desc_32 *kernel_tss_gdt = &gdt[KERNEL_TSS_SELECTOR / 8];
    kernel_tss_gdt->seg_limit_15_0 = sizeof(kernel_tss) - 1;
    kernel_tss_gdt->base_15_0 = ((uintptr_t)&kernel_tss) & 0xffff;
    kernel_tss_gdt->base_23_16 = (((uintptr_t)&kernel_tss) >> 16) & 0xff;
    kernel_tss_gdt->base_31_24 = (((uintptr_t)&kernel_tss) >> 24) & 0xff;

    // use ltr to load the kernel task register
    __asm__ volatile("ltr %0" :: "r"((uint16_t)KERNEL_TSS_SELECTOR) : "memory");
}

static int alloc_tss_slot(void) {
    for (int i = 0; i < TASK_TSS_SELECTOR_COUNT; i++) {
        uint32_t mask = (1u << i);
        if ((tss_slot_bitmap & mask) == 0) {
            tss_slot_bitmap |= mask;
            return i;
        }
    }
    return -1;
}

static void free_tss_slot(int slot) {
    tss_slot_bitmap &= ~(1u << slot);
}

static void set_tss_gdt_entry(unsigned int tss_slot, struct x86_tss *tss) {
    struct x86_desc_32 *tss_gdt = &gdt[TASK_TSS_SELECTOR_BASE / 8 + tss_slot];

    tss_gdt->seg_limit_15_0 = sizeof(*tss) - 1;
    tss_gdt->base_15_0 = ((uintptr_t)tss) & 0xffff;
    tss_gdt->base_23_16 = (((uintptr_t)tss) >> 16) & 0xff;
    tss_gdt->base_31_24 = (((uintptr_t)tss) >> 24) & 0xff;
    tss_gdt->p_dpl_s_type = 0b10001001;       // P(1) DPL(00) S(0) 1 0 B(0) 1
    tss_gdt->g_db_seg_limit_19_16 = 0b10000000;       // G(1) 0 0 0 limit 19:16
}

__asm__(
    "test_func:\n"
    "ljmp $0x28,$0\n"
    "ret\n"
);

void tss_test(void) {
    printf("top of tss_test\n");

    // set up a temporary task, switch to it and back
    static struct x86_tss test_tss;
    extern void test_func(void);
    int slot;
    x86_init_task_tss(&test_tss, &slot, (uintptr_t)&test_func, 0);

    set_tss_gdt_entry(slot, &test_tss);
    printf("tss is set up using slot %d, switching\n", slot);

    // put a far pointer on the stack and branch to it
    struct {
        uint32_t addr;
        uint16_t seg;
    } __PACKED addr;
    addr.seg = TASK_TSS_SELECTOR_BASE + slot * 8;
    addr.addr = 0;

    __asm__ volatile("ljmp *%0\n" :: "m"(addr) : "memory");
    printf("switched back\n");

    free_tss_slot(slot);

    printf("bottom of tss_test\n");
}

int x86_init_task_tss(struct x86_tss *tss, int *tss_slot, uintptr_t entry_point, uint32_t sp) {
    *tss_slot = alloc_tss_slot();
    if (*tss_slot < 0)
        return -1;

    memset(tss, 0, sizeof(*tss));

    tss->eflags = (1<<9); // interrupts enabled
    tss->eip = entry_point;
    tss->esp = sp;
    tss->cs = CODE_SELECTOR;
    tss->ss = DATA_SELECTOR;
    tss->ds = DATA_SELECTOR;
    tss->es = DATA_SELECTOR;
    tss->fs = DATA_SELECTOR;
    tss->gs = DATA_SELECTOR;

    set_tss_gdt_entry(*tss_slot, tss);

    return 0;
}

void x86_task_switch_to(struct task *task) {
    //printf("x86 switch to %p\n", task);

    struct {
        uint32_t addr;
        uint16_t seg;
    } __PACKED addr;
    addr.seg = TASK_TSS_SELECTOR_BASE + task->tss_slot * 8;
    addr.addr = 0;

    __asm__ volatile("ljmp *%0\n" :: "m"(addr) : "memory");
}

