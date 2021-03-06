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
#pragma once

#include <list.h>
#include <sys/types.h>
#include <x86/x86.h>

typedef struct task {
    struct list_node node;

    enum state {
        INITIAL,
        READY,
        RUNNING,
        DEAD
    } state;

    int critical_section_count;

    void (*entry)(void *);
    void *arg;

    uintptr_t stack;
    size_t stack_size;

    uintptr_t saved_sp;
} task_t;

void task_init(void);
void task_become_idle(void);

status_t task_create(task_t *t, const char *name, void (*entry)(void *), void *arg, uintptr_t stack, size_t stack_size);
status_t task_start(task_t *t);
void task_exit(void) __NO_RETURN;
void task_reschedule(void);

// manipulate a counter per task that disable/enables irqs
void enter_critical_section(void);
void exit_critical_section(void);

