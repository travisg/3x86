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

#include <stdint.h>

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

// System Selectors
#define NULL_SELECTOR       0x00

// x86 selectors
#define CODE_SELECTOR       0x08
#define DATA_SELECTOR       0x10
#define USER_CODE_32_SELECTOR   (0x18 | 3)
#define USER_DATA_32_SELECTOR   (0x20 | 3)

// functions
void x86_init(void);
