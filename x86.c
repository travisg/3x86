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
#include <x86.h>

#include <compiler.h>
#include <stdint.h>
#include <stdlib.h>

// 32bit generic descriptors
struct desc_32 {
    uint16_t seg_limit_15_0;
    uint16_t base_addres_15_0;
    uint8_t  base_addres_23_16;
    uint8_t  p_dpl_s_type;
    uint8_t  g_db_seg_limit_19_16;
    uint8_t  base_31_24;
} __PACKED;

struct desc_ptr {
    uint16_t len;
    uint32_t ptr;
} __PACKED;

static struct desc_32 gdt[] = {
    { 0 }, // null descriptor
    {
        // ring 0 code segment 0x8
        0xffff,           // limit 15:00
        0x0000,           // base 15:00
        0x00,             // base 23:16
        0b10011010,       // P(1) DPL(00) S(1) 1 C(0) R(1) A(0)
        0b11001111,       // G(1) D(1) 0 0 limit 19:16
        0x0               // base 31:24
    },
    {
        // ring 0 data segment 0x10
        0xffff,           // limit 15:00
        0x0000,           // base 15:00
        0x00,             // base 23:16
        0b10010010,       // P(1) DPL(00) S(1) 0 E(0) W(1) A(0)
        0b11001111,       // G(1) D(1) 0 0 limit 19:16
        0x0               // base 31:24
    },
    {
        // ring 3 code segment 0x1b (0x18 | 3)
        0xffff,           // limit 15:00
        0x0000,           // base 15:00
        0x00,             // base 23:16
        0b11111010,       // P(1) DPL(11) S(1) 1 C(0) R(1) A(0)
        0b11001111,       // G(1) D(1) 0 0 limit 19:16
        0x0               // base 31:24
    },
    {
        // ring 3 data segment 0x23 (0x20 | 3)
        0xffff,           // limit 15:00
        0x0000,           // base 15:00
        0x00,             // base 23:16
        0b11110010,       // P(1) DPL(11) S(1) 0 E(0) W(1) A(0)
        0b11001111,       // G(1) D(1) 0 0 limit 19:16
        0x0               // base 31:24
    },
};

// gate descriptors
struct gate_desc_32 {
    uint16_t seg_offset_15_0;
    uint16_t seg;
    uint8_t  param_count;
    uint8_t  p_dpl_type;
    uint16_t seg_offset_31_16;
} __PACKED;

static struct gate_desc_32 idt[256];

__attribute__((interrupt))
void x86_exception_entry(void *iframe, unsigned int error_code) {
    printf("exception entry: iframe %p, error_code %u\n", iframe, error_code);
    for (;;);
}

__attribute__((interrupt))
void x86_irq_entry(void *iframe) {
    printf("irq entry: iframe %p\n", iframe);
    for (;;);
}

// early cpu initialization
void x86_init(void) {
    // switch to our GDT
    struct desc_ptr gdt_ptr;
    gdt_ptr.len = sizeof(gdt) - 1;
    gdt_ptr.ptr = (uint32_t)gdt;
    __asm__ volatile("lgdt %0" :: "m"(gdt_ptr) : "memory");

    // reload our segments
    __asm__ volatile(
        "mov  %0, %%ds;"
        "mov  %0, %%es;"
        "mov  %0, %%fs;"
        "mov  %0, %%gs;"
        :: "r"(DATA_SELECTOR)
        : "memory");

    // long branch to the next line to reload cs
    __asm__ volatile(
        "ljmp  %0,$0f;"
        "0:;"
        :: "i"(CODE_SELECTOR));

    // initialize the idt
    for (int i = 0; i < 256; i++) {
        uintptr_t target = (i < 32) ? (uintptr_t)x86_exception_entry : (uintptr_t)x86_irq_entry;
        idt[i].seg_offset_15_0 = target & 0xffff;
        idt[i].seg = CODE_SELECTOR;
        idt[i].param_count = 0; // unused on int and trap gates
        idt[i].p_dpl_type = 0b10001110; // present, dpl 0, type E - 32bit interrupt gate
        idt[i].seg_offset_31_16 = (target >> 16) & 0xffff;
    }

    // load the IDT
    struct desc_ptr idt_ptr;
    idt_ptr.len = sizeof(idt) - 1;
    idt_ptr.ptr = (uint32_t)idt;
    __asm__ volatile("lidt %0" :: "m"(idt_ptr) : "memory");

    // trap
    volatile int a = 5 / 0;
}

