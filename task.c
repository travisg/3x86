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
#include <task.h>

#include <stdio.h>
#include <string.h>
#include <x86/x86.h>

static task_t idle_task;
static uint8_t idle_stack[256];

static task_t *current_task;

static task_t *slot_a;
static task_t *slot_b;

static void idle_loop(void *arg) {
    for (;;) {
        x86_hlt();
    }
}

static void task_trampoline(void) {
    x86_sti();

    printf("top of task %p\n", current_task);
    current_task->entry(current_task->arg);
    task_exit();
}

void task_exit(void) {
    printf("exiting task %p\n", current_task);
    // XXX actually do it
    for (;;);
}

void task_init(void) {
    printf("initializing tasks\n");

    task_create(&idle_task, "idle", &idle_loop, 0, (uintptr_t)idle_stack, sizeof(idle_stack));

    // HACK to put us in some sort of context
    static task_t initial_kernel_task;
    current_task = &initial_kernel_task;
}

status_t task_create(task_t *t, const char *name, void (*entry)(void *), void *arg, uintptr_t stack, size_t stack_size) {
    t->entry = entry;
    t->arg = arg;

    t->stack = stack;
    t->stack_size = stack_size;

    if (x86_init_task(t, (uintptr_t)task_trampoline, t->stack + t->stack_size) < 0) {
        return -1;
    }

    return 0;
}

status_t task_start(task_t *t) {
    if (!slot_a) {
        slot_a = t;
    } else if (!slot_b) {
        slot_b = t;
    }

    return 0;
}

void task_reschedule(void) {
    x86_flags_t flags = x86_irq_disable();

    task_t *old_task = current_task;
    task_t *next_task;

    if (current_task == slot_a) {
        next_task = slot_b;
    } else {
        next_task = slot_a;
    }

    current_task = next_task;
    x86_task_switch(old_task, next_task);

    x86_irq_restore(flags);
}

