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
#include <string.h>

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t count) {
    void *olddest = dest;
    __asm__ volatile(
        "cld;"
        "rep movsb"
        : "=D"(dest), "=S"(src), "=c"(count)
        : "0" (dest), "1" (src), "2" (count)
        : "memory", "cc");
    return olddest;
}

void * memset(void *dest, int c, size_t count) {
    void *olddest = dest;
    __asm__ volatile(
        "cld;"
        "rep stosb"
        : "=D"(dest), "=c"(count)
        : "0" (dest), "1" (count), "a"(c)
        : "memory", "cc");
    return olddest;
}

size_t strlen(char const *s) {
    size_t i = 0;

    while (s[i]) {
        i+= 1;
    }

    return i;
}
