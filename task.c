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

#include <compiler.h>
#include <stdio.h>
#include <string.h>
#include <trace.h>
#include <x86/x86.h>

#define LOCAL_TRACE 0

static task_t idle_task;
uint8_t idle_stack[512] __ALIGNED(4);

static task_t *current_task;
static struct list_node run_queue;

// called once at boot by the initial start routine to exit the single threaded phase
// of bootup and start the scheduler.
void task_become_idle(void) {
    printf("starting multitasking\n");

    // kick off the scheduler once
    task_reschedule();

    // drop into idle loop
    x86_sti();
    for (;;) {
        x86_hlt();
    }
}

// called once at startup to initialize the tasking subsystem
void task_init(void) {
    LTRACEF("initializing tasks\n");

    list_initialize(&run_queue);

    // create the idle task and set it as the current
    task_create(&idle_task, "idle", NULL, 0, (uintptr_t)idle_stack, sizeof(idle_stack));
    current_task = &idle_task;
}

// initial routine run by every new task
static void task_trampoline(void) {
    // reenable interrupts, since they were implicitly disabled by the reschedule routine
    x86_sti();

    LTRACEF("top of task %p\n", current_task);

    // call the entry point
    current_task->entry(current_task->arg);

    // fall through to exit
    task_exit();
}

void task_exit(void) {
    LTRACEF("exiting task %p\n", current_task);

    x86_cli();

    // set ourselves to the DEAD state and reschedule
    // the scheduler wont put us back in the run queue
    current_task->state = DEAD;
    task_reschedule();

    // never get here
    __UNREACHABLE;
}

status_t task_create(task_t *t, const char *name, void (*entry)(void *), void *arg, uintptr_t stack, size_t stack_size) {
    // initialize the sructure
    list_clear_node(&t->node);
    t->state = INITIAL;
    t->entry = entry;
    t->arg = arg;
    t->stack = stack;
    t->stack_size = stack_size;

    // get the x86 layer to initialize its part
    status_t err = x86_init_task(t, (uintptr_t)task_trampoline, t->stack + t->stack_size);
    if (err < 0) {
        return err;
    }

    return 0;
}

// move the task from the initial state and put it in the run queue
status_t task_start(task_t *t) {
    x86_flags_t flags = x86_irq_disable();

    if (t->state != INITIAL) {
        x86_irq_restore(flags);
        return -1;
    }

    t->state = READY;
    list_add_head(&run_queue, &t->node);

    x86_irq_restore(flags);

    return 0;
}

// see if a new task is ready to run
void task_reschedule(void) {
    x86_flags_t flags = x86_irq_disable();

    task_t *old_task = current_task;
    task_t *next_task;

    // if the old one is running, put it back in the run queue
    if (current_task->state == RUNNING) {
        current_task->state = READY;
        list_add_tail(&run_queue, &current_task->node);
    }

    // find a new thread to run from the run queue
    next_task = list_remove_head_type(&run_queue, struct task, node);
    if (!next_task) {
        // if nothing in the queue, pick the idle task
        next_task = &idle_task;
    }

    if (next_task == old_task) {
        goto out;
    }

    current_task = next_task;
    current_task->state = RUNNING;
    x86_task_switch(old_task, next_task);

out:
    x86_irq_restore(flags);
}

