/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#ifndef __STDLIB_H
#define __STDLIB_H

#include <sys/types.h>
#include <stddef.h>
#include <malloc.h>
#include <printf.h>
#include <endian.h>
#include <arch/defines.h>

unsigned gcd(unsigned m, unsigned n);
unsigned lcm(unsigned m, unsigned n);
int atoi(const char *num);
unsigned int atoui(const char *num);
long atol(const char *num);
unsigned long atoul(const char *num);
unsigned long int strtoul(const char *nptr, char **endptr, int base);
char *getenv(const char *name);
void qsort(void *base, size_t nmemb, size_t size,
	   int (*compar)(const void *, const void *));

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDDOWN(a, b) ((a) & ~((b)-1))

/* allocate a buffer on the stack aligned and padded to the cpu's cache line size */
#define STACKBUF_DMA_ALIGN(var, size) \
	uint8_t __##var[(size) + CACHE_LINE]; uint8_t *var = (uint8_t *)(ROUNDUP((addr_t)__##var, CACHE_LINE))
/*
 * Macro to allocate buffer in both local & global space, the STACKBUF_DMA_ALIGN
 * cannot  be used for global space. If we use STACKBUF_DMA_ALIGN 'C' compiler
 * throws the error "Initializer element is not constant", since global variable
 * need to be initialized with a constant value.
 */
#define BUF_DMA_ALIGN(var, size) \
	        static uint8_t var[ROUNDUP(size, CACHE_LINE)] __attribute__((aligned(CACHE_LINE)));

#endif

