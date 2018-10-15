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
#include <hw/pit.h>

#include <stdio.h>
#include <hw/pc.h>
#include <hw/pic.h>
#include <x86/x86.h>

// driver for the 8253/8254 timer chip present on legacy PCs

#define PIT_DATA0       (0x40)  // general use channel
#define PIT_DATA1       (0x41)  // (legacy) used for DRAM refresh
#define PIT_DATA2       (0x42)  // PC speaker
#define PIT_CMD         (0x43)  // command register

#define PIT_FREQ        1193182 // input frequency in Hz
#define PIT_HZ          100     // tick rate we desire
#define PIT_COUNTDOWN   11932   // PIT_FREQ / PIT_HZ rounded up

static uint32_t timer_ticks;

uint16_t pit_read_count(void) {
    uint16_t val;

    x86_flags_t flags = x86_irq_disable();

    outp(PIT_CMD, (0 << 6)); // latch channel 0
    val = inp(PIT_DATA0);
    val |= inp(PIT_DATA0) << 8;

    x86_irq_restore(flags);

    return val;
}

void pit_init(void) {
    x86_flags_t flags = x86_irq_disable();

    // initialize channel 0 to 100Hz continuous countdown using mode 2
    uint8_t val;
    val = (0 << 6) | // channel 0
          (3 << 4) | // access mode lobyte/hibyte
          (2 << 1) | // mode 2 (rate generator)
          0;         // 16 bit binary mode
    outp(PIT_CMD, val);

    // compute the countdown value and reload the reload register
    outp(PIT_DATA0, (uint8_t)PIT_COUNTDOWN);
    outp(PIT_DATA0, PIT_COUNTDOWN >> 8);

    // eoi and unmask the timer irq
    pic_send_eoi(IRQ_PIT);
    pic_set_mask(IRQ_PIT, false);

    x86_irq_restore(flags);
}

// ticks at 100Hz
void pit_irq(void) {
    timer_ticks++;
}

uint32_t current_time(void) {
    return timer_ticks * 10; // time in ms
}
