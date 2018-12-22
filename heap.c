/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#include <heap.h>

#include <miniheap.h>
#include <string.h>
#include <trace.h>

#define LOCAL_TRACE 0
#define HEAP_TRACE 0

// heap wrapper routines

static uint32_t default_heap[16384/sizeof(uint32_t)];

void heap_init(void) {
    miniheap_init(default_heap, sizeof(default_heap));
}

void heap_trim(void) {
    miniheap_trim();
}

void *malloc(size_t size) {
    LTRACEF("size %zd\n", size);

    void *ptr = miniheap_alloc(size, 0);
    if (HEAP_TRACE) {
        printf("caller %p malloc %zu -> %p\n", __GET_CALLER(), size, ptr);
    }
    return ptr;
}

void *memalign(size_t boundary, size_t size) {
    LTRACEF("boundary %zu, size %zd\n", boundary, size);

    void *ptr = miniheap_alloc(size, boundary);
    if (HEAP_TRACE) {
        printf("caller %p memalign %zu, %zu -> %p\n", __GET_CALLER(), boundary, size, ptr);
    }
    return ptr;
}

void *calloc(size_t count, size_t size) {
    LTRACEF("count %zu, size %zd\n", count, size);

    size_t realsize = count * size;

    void *ptr = miniheap_alloc(realsize, 0);
    if (likely(ptr)) {
        memset(ptr, 0, realsize);
    }

    if (HEAP_TRACE) {
        printf("caller %p calloc %zu, %zu -> %p\n", __GET_CALLER(), count, size, ptr);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    LTRACEF("ptr %p, size %zd\n", ptr, size);

    void *ptr2 = miniheap_realloc(ptr, size);
    if (HEAP_TRACE) {
        printf("caller %p realloc %p, %zu -> %p\n", __GET_CALLER(), ptr, size, ptr2);
    }
    return ptr2;
}

void free(void *ptr) {
    LTRACEF("ptr %p\n", ptr);
    if (HEAP_TRACE) {
        printf("caller %p free %p\n", __GET_CALLER(), ptr);
    }

    miniheap_free(ptr);
}

void heap_dump(void) {
    miniheap_dump();
}

