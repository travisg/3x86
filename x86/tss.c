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
