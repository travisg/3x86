/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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
#include <asm.h>

// define a table of up to 256 interrupt routines that push the irq number
// and branch to a common routine.
//
// each routine is precisely 16 bytes apart and the idt construction code
// takes this into account.
.section .text.isr

.balign 16
FUNCTION(_isr_table)

.set i, 0
.rept NUM_INT

.if i == 8 || (i >= 10 && i <= 14) || i == 17
    // error code is pushed by hardware
.else
    pushl $0                // fill in error code in iframe
.endif
    pushl $i                // interrupt number
    jmp x86_interrupt_common

.balign 16

.set i, i + 1
.endr

// all exceptions funnel through here
FUNCTION(x86_interrupt_common)
    pushl %gs               // save segment registers
    pushl %fs
    pushl %es
    pushl %ds
    pusha                   // save general purpose registers
    movl $DATA_SELECTOR, %eax // put known good value in segment registers
    movl %eax, %gs
    movl %eax, %fs
    movl %eax, %es
    movl %eax, %ds

    // call the C level exception handling code using fastcall
    movl %esp, %ecx         // store pointer to iframe
    call x86_exception_handler

    popa                    // restore general purpose registers
    popl %ds                // restore segment registers
    popl %es
    popl %fs
    popl %gs
    addl $8, %esp           // drop exception number and error code
    iret

