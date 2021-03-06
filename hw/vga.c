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
#include <hw/vga.h>

#include <ctype.h>
#include <console.h>
#include <string.h>
#include <stdlib.h>
#include <x86/x86.h>

// mini driver for vga text mode console

static unsigned short *vga = (void *)0xb8000;
static unsigned col = 0;
static unsigned line = 0;

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

void vga_console_init(bool clear) {
    // disable the cursor
    outp(0x3d4, 0x0a);
    outp(0x3d5, 0x20);

    if (clear) {
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            vga[i] = 0x720;
        }
    }
}

// scroll the screen by copying line 1-24 to line 0-23
// and then clearing the last line
static void vga_console_scrup(void) {
    memcpy(vga, vga + SCREEN_WIDTH, (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2);
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        vga[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + i] = 0x720;
    }
}

void vga_console_putchar(char c) {
    switch (c) {
        case '\n':
            line++;
            break;
        case '\r':
            col = 0;
            break;
        case '\t':
            col = ROUNDUP(col + 1, 4);
            break;
        default:
            if (isprint(c)) {
                vga[line * SCREEN_WIDTH + col] = 0xf00 | (c & 0x7f);
                col++;
            }
    }

    if (col >= SCREEN_WIDTH) {
        col = 0;
        line++;
    }
    if (line == SCREEN_HEIGHT) {
        vga_console_scrup();
        line--;
    }
}
