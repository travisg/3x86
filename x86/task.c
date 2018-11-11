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

int x86_init_task(struct task *task, uintptr_t entry_point, uint32_t sp) {
    struct context_switch_frame *frame = (void *)(sp - sizeof(struct context_switch_frame));
    frame->edi = frame->esi = frame->ebp = frame->ebx = 0;
    frame->return_address = entry_point;

    task->saved_sp = (uint32_t)frame;
    //printf("initializing task %p, entry %#lx, sp %#lx, saved sp %#lx\n", task, entry_point, sp, task->saved_sp);

    return 0;
}

void x86_task_switch(struct task *old, struct task *task) {
    //printf("x86 switch from %p to %p (new saved sp %#lx)\n", old, task, task->saved_sp);

    x86_asm_switch(&old->saved_sp, task->saved_sp);
}

