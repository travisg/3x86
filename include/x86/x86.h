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

// System Selectors
#define NULL_SELECTOR       0x00

// x86 selectors
#define CODE_SELECTOR       0x08
#define DATA_SELECTOR       0x10
#define USER_CODE_32_SELECTOR   (0x18 | 3)
#define USER_DATA_32_SELECTOR   (0x20 | 3)
#define KERNEL_TSS_SELECTOR 0x28

// only implement 0x30 interrupts for now
#define NUM_INT             0x30

#ifndef __ASSEMBLER__

#include <compiler.h>
#include <stdint.h>

// 32bit generic descriptors
struct x86_desc_32 {
    uint16_t seg_limit_15_0;
    uint16_t base_15_0;
    uint8_t  base_23_16;
    uint8_t  p_dpl_s_type;
    uint8_t  g_db_seg_limit_19_16;
    uint8_t  base_31_24;
} __PACKED;

// gate descriptors
struct x86_gate_desc_32 {
    uint16_t seg_offset_15_0;
    uint16_t seg;
    uint8_t  param_count;
    uint8_t  p_dpl_type;
    uint16_t seg_offset_31_16;
} __PACKED;

struct x86_desc_ptr {
    uint16_t len;
    uint32_t ptr;
} __PACKED;

struct x86_iframe {
    uint32_t di, si, bp, sp, bx, dx, cx, ax;            // pushed by common handler using pusha
    uint32_t ds, es, fs, gs;                            // pushed by common handler
    uint32_t vector;                                    // pushed by stub
    uint32_t err_code;                                  // pushed by interrupt or stub
    uint32_t ip, cs, flags;                             // pushed by interrupt
    uint32_t user_sp, user_ss;                          // pushed by interrupt if priv change occurs
};

struct x86_tss {
    uint16_t prev_tss;
    uint16_t __pad_0;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t __pad_1;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t __pad_2;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t __pad_3;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t __pad_4;
    uint16_t cs;
    uint16_t __pad_5;
    uint16_t ss;
    uint16_t __pad_6;
    uint16_t ds;
    uint16_t __pad_7;
    uint16_t fs;
    uint16_t __pad_8;
    uint16_t gs;
    uint16_t __pad_9;
    uint16_t ldt;
    uint16_t __pad_10;
    uint16_t debug_trap;
    uint16_t io_map_base;
} __PACKED;

_Static_assert(sizeof(struct x86_tss) == 0x68, "");

static inline void x86_clts(void) {__asm__ __volatile__ ("clts"); }
static inline void x86_hlt(void) {__asm__ __volatile__ ("hlt"); }
static inline void x86_sti(void) {__asm__ __volatile__ ("sti"); }
static inline void x86_cli(void) {__asm__ __volatile__ ("cli"); }
static inline void x86_ltr(uint16_t sel) { __asm__ __volatile__ ("ltr %%ax" :: "a" (sel)); }

static inline uint32_t x86_get_cr0(void) {
    uint32_t rv;

    __asm__ __volatile__ (
        "mov %%cr0, %0 \n\t"
        : "=r" (rv));
    return rv;
}
static inline void x86_set_cr0(uint32_t in_val) {
    __asm__ __volatile__ (
        "mov %0,%%cr0 \n\t"
        :
        :"r" (in_val));
}

static inline uint32_t x86_get_cr2(void) {
    uint32_t rv;

    __asm__ __volatile__ (
        "mov %%cr2, %0"
        : "=r" (rv));
    return rv;
}

static inline uint32_t x86_get_cr3(void) {
    uint32_t rv;

    __asm__ __volatile__ (
        "mov %%cr3, %0"
        : "=r" (rv));
    return rv;
}

static inline void x86_set_cr3(uint32_t in_val) {
    __asm__ __volatile__ (
        "mov %0,%%cr3 \n\t"
        :
        :"r" (in_val));
}
typedef uint32_t x86_flags_t;

static inline uint32_t x86_save_flags(void) {
    unsigned int state;

    __asm__ volatile(
        "pushfl;"
        "popl %0"
        : "=rm" (state)
        :: "memory");

    return state;
}

static inline void x86_restore_flags(uint32_t flags) {
    __asm__ volatile(
        "pushl %0;"
        "popfl"
        :: "g" (flags)
        : "memory", "cc");
}

static inline uint8_t inp(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0"
                          : "=a" (rv)
                          : "d" (_port));
    return (rv);
}

static inline uint16_t inpw (uint16_t _port) {
    uint16_t rv;
    __asm__ __volatile__ ("inw %1, %0"
                          : "=a" (rv)
                          : "d" (_port));
    return (rv);
}

static inline uint32_t inpd(uint16_t _port) {
    uint32_t rv;
    __asm__ __volatile__ ("inl %1, %0"
                          : "=a" (rv)
                          : "d" (_port));
    return (rv);
}

static inline void outp(uint16_t _port, uint8_t _data) {
    __asm__ __volatile__ ("outb %1, %0"
                          :
                          : "d" (_port),
                          "a" (_data));
}

static inline void outpw(uint16_t _port, uint16_t _data) {
    __asm__ __volatile__ ("outw %1, %0"
                          :
                          : "d" (_port),
                          "a" (_data));
}

static inline void outpd(uint16_t _port, uint32_t _data) {
    __asm__ __volatile__ ("outl %1, %0"
                          :
                          : "d" (_port),
                          "a" (_data));
}

// global data
extern struct x86_desc_32 gdt[];
extern struct x86_tss kernel_tss;

// functions
void x86_init(void);
void x86_tss_init(void);

#endif
