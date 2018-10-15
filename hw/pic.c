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
#include <hw/pic.h>

#include <stdio.h>
#include <hw/pc.h>
#include <hw/pit.h>
#include <x86/x86.h>

// driver for the pair of 8259a interrupt controllers present on legacy PCs
// taken largely from https://wiki.osdev.org/PIC

#define PIC1_BASE (0x20)
#define PIC2_BASE (0xA0)

#define PIC1_CMD  (PIC1_BASE)
#define PIC1_DATA (PIC1_BASE + 1)
#define PIC2_CMD  (PIC2_BASE)
#define PIC2_DATA (PIC2_BASE + 1)

#define ICW1_ICW4       0x01        /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02        /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04        /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08        /* Level triggered (edge) mode */
#define ICW1_INIT       0x10        /* Initialization - required! */

#define ICW4_8086       0x01        /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02        /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08        /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C        /* Buffered mode/master */
#define ICW4_SFNM       0x10        /* Special fully nested (not) */

#define PIC_EOI         0x20        /* End-of-interrupt command code */

static inline void io_wait(void) {
    // Port 0x80 is used for 'checkpoints' during POST.
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

// arguments:
//     offset1 - vector offset for master PIC
//         vectors on the master become offset1..offset1+7
//     offset2 - same for slave PIC: offset2..offset2+7
static void pic_remap(int offset1, int offset2) {
    unsigned char a1, a2;

    a1 = inp(PIC1_DATA);                      // save masks
    a2 = inp(PIC2_DATA);

    outp(PIC1_CMD, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    io_wait();
    outp(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outp(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
    io_wait();
    outp(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
    io_wait();
    outp(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    outp(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    outp(PIC1_DATA, ICW4_8086);
    io_wait();
    outp(PIC2_DATA, ICW4_8086);
    io_wait();

    outp(PIC1_DATA, a1);   // restore saved masks.
    outp(PIC2_DATA, a2);
}

void pic_init(void) {
    // reinitialize the PIC controllers, giving them specified vector offsets
    // rather than 8h and 70h, as configured by default
    pic_remap(0x20, 0x28);

    // mask everything
    outp(PIC1_DATA, 0xff);
    outp(PIC2_DATA, 0xff);
}

void pic_set_mask(unsigned char irq, bool set) {
    uint16_t port;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    uint8_t val = inp(port);
    if (set) {
        val |= (1 << irq);
    } else {
        val &= ~(1 << irq);
    }
    outp(port, val);
}

void pic_send_eoi(unsigned char irq) {
    if(irq >= 8) {
        outp(PIC2_CMD, PIC_EOI);
    }

    outp(PIC1_CMD, PIC_EOI);
}

// general top level irq routine for IRQs in the PIC range
void pic_irq(unsigned int vector) {
    pic_send_eoi(vector);
    switch (vector) {
        case IRQ_PIT:
            pit_irq();
            break;
        default:
            printf("unhandled PIC interrupt\n");
            break;
    }
}
