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
#include <console.h>

#include <string.h>

static unsigned short *vga = (void *)0xb8000;
static unsigned col = 0;
static unsigned line = 0;

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

void console_init(void) {
}

static void scrup(void) {
    memcpy(vga, vga + SCREEN_WIDTH, (SCREEN_HEIGHT - 1) * SCREEN_HEIGHT * 2);
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        vga[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + i] = 0x720;
    }
}

void putchar(int c) {

    switch (c) {
        case '\n':
            line++;
            break;
        case '\r':
            col = 0;
            break;
        default:
            vga[line * SCREEN_WIDTH + col] = 0xf00 | (c & 0x7f);
            col++;
    }

    if (col == SCREEN_WIDTH) {
        col = 0;
        line++;
        if (line == SCREEN_HEIGHT) {
            scrup();
            line--;
        }
    }
}

