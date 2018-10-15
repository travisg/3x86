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

#include <compiler.h>
#include <stdint.h>
#include <stdlib.h>
#include <hw/pic.h>

struct x86_desc_32 gdt[GDT_COUNT] = {
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
    {
        // kernel TSS descriptor (0x28)
        sizeof(kernel_tss),           // limit 15:00
        0x0000,           // base 15:00
        0x00,             // base 23:16
        0b10001001,       // P(1) DPL(00) S(0) 1 0 B(0) 1
        0b10000000,       // G(1) 0 0 0 limit 19:16
        0x0               // base 31:24
    },
    // 0x30+ are available user TSS descriptors
};

static struct x86_gate_desc_32 idt[NUM_INT];

// declared in exceptions.S
extern void x86_exception_entry(void);
extern void x86_irq_entry(void);
extern void _isr_table(void);

// early cpu initialization
void x86_init(void) {
    // switch to our GDT
    struct x86_desc_ptr gdt_ptr;
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
    uintptr_t target = (uintptr_t)_isr_table;
    for (int i = 0; i < NUM_INT; i++) {
        idt[i].seg_offset_15_0 = target & 0xffff;
        idt[i].seg = CODE_SELECTOR;
        idt[i].param_count = 0; // unused on int and trap gates
        idt[i].p_dpl_type = 0b10001110; // present, dpl 0, type E - 32bit interrupt gate
        idt[i].seg_offset_31_16 = (target >> 16) & 0xffff;

        // each irq veneer routine is exactly 16 bytes long to make it easy
        // to walk through the table here.
        // irq veneers are implemented in exceptions.S
        target += 16;
    }

    // load the IDT
    struct x86_desc_ptr idt_ptr;
    idt_ptr.len = sizeof(idt) - 1;
    idt_ptr.ptr = (uint32_t)idt;
    __asm__ volatile("lidt %0" :: "m"(idt_ptr) : "memory");

    // switch to the kernel tss
    x86_tss_init();
}

static void dump_fault_frame(struct x86_iframe *frame) {
    printf(" CS:     %04lx EIP: %08lx EFL: %08lx CR2: %08lx\n",
           frame->cs, frame->ip, frame->flags, x86_get_cr2());
    printf("EAX: %08lx ECX: %08lx EDX: %08lx EBX: %08lx\n",
           frame->ax, frame->cx, frame->dx, frame->bx);
    printf("ESP: %08lx EBP: %08lx ESI: %08lx EDI: %08lx\n",
           frame->sp, frame->bp, frame->si, frame->di);
    printf(" DS:     %04lx  ES:     %04lx  FS:   %04lx    GS:     %04lx\n",
           frame->ds, frame->es, frame->fs, frame->gs);
}

__NO_RETURN
static void exception_die(struct x86_iframe *frame, const char *msg) {
    printf(msg);
    dump_fault_frame(frame);

    for (;;) {
        x86_cli();
        x86_hlt();
    }
}

void x86_exception_handler(struct x86_iframe *iframe) {

    switch (iframe->vector) {
        case 0x20 ... 0x2f: // PIC interrupts
            pic_irq(iframe->vector - 0x20);
            break;
        default:
            printf("vector %lu (%#lx), err code %#lx\n", iframe->vector, iframe->vector, iframe->err_code);
            exception_die(iframe, "unhandled exception\n");
    }
}

