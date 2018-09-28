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

struct x86_tss kernel_tss;

void x86_tss_init(void) {
    // initialize the kernel tss and switch to it

    // punch in the address of the kernel_tss
    struct x86_desc_32 *kernel_tss_gdt = &gdt[KERNEL_TSS_SELECTOR / 8];
    kernel_tss_gdt->seg_limit_15_0 = sizeof(kernel_tss) - 1;
    kernel_tss_gdt->base_15_0 = ((uintptr_t)&kernel_tss) & 0xffff;
    kernel_tss_gdt->base_23_16 = (((uintptr_t)&kernel_tss) >> 16) & 0xff;
    kernel_tss_gdt->base_31_24 = (((uintptr_t)&kernel_tss) >> 24) & 0xff;

    // use ltr to load the kernel task register
    __asm__ volatile("ltr %0" :: "r"((uint16_t)KERNEL_TSS_SELECTOR) : "memory");
}

void init_task_tss(struct x86_tss *tss, uintptr_t entry_point, uint32_t sp) {
    memset(tss, 0, sizeof(*tss));

    tss->eip = entry_point;
    tss->esp = sp;
    tss->cs = CODE_SELECTOR;
    tss->ss = DATA_SELECTOR;
    tss->ds = DATA_SELECTOR;
    tss->es = DATA_SELECTOR;
    tss->fs = DATA_SELECTOR;
    tss->gs = DATA_SELECTOR;
}

void set_tss_gdt_entry(unsigned int tss_slot, struct x86_tss *tss) {
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
    init_task_tss(&test_tss, (uintptr_t)&test_func, 0);
    set_tss_gdt_entry(0, &test_tss);
    printf("tss is set up, switching\n");

    __asm__ volatile("ljmp %0,$0\n" :: "i"(TASK_TSS_SELECTOR_BASE) : "memory");
    printf("switched back\n");

    printf("bottom of tss_test\n");
}
