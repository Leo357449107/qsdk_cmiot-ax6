/* Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdio.h>
#ifdef INCLUDE_BREAKPAD
#include "breakpad_qcawrapper.h"
#endif

void ApplicationCallback(void)
{
    printf("\n Inside Application Callback \n");
}

void crash() {
    volatile int* a = (int*)(NULL);
    *a = 1;
}

int main(void)
{
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler(&ApplicationCallback);
#endif
    printf("\nBreakpad Test Application About to Crash!\n\n");
    crash();
    return 0;
}
