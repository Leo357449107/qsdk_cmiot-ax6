/* Copyright (c) 2019, The Linux Foundation. All rights reserved.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "breakpad_qcawrapper.h"
#include "client/linux/handler/exception_handler.h"
#include <stdio.h>

using namespace google_breakpad;

static bool breakpadDumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
    /* Performing the Desired Exit Process */
    printf("Performing Breakpad Callback .....................................\n");
    printf("Dump File Location: %s\n", descriptor.path());

    ApplnCallback applnCallback = (ApplnCallback)context;
    if(applnCallback)
    {
        printf("Performing Application Specific Callback .....................\n");
        (*applnCallback)();
    }

    return succeeded;
}

void breakpad_ExceptionHandler(ApplnCallback applnCallback)
{
    static google_breakpad::ExceptionHandler* exceptHandler = NULL;
    if (exceptHandler)
    {
        fprintf(stderr, "Handler is not NULL");
        return;
    }
    exceptHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor("/tmp"), NULL, breakpadDumpCallback, (void*)applnCallback, true, -1);
}
