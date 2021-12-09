/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#ifndef __APP_H
#define __APP_H
#include <stdlib.h>

/* app support api */
void apps_init(void); /* one time setup */

/* app entry point */
struct app_descriptor;
typedef void (*app_init)(const struct app_descriptor *);
typedef void (*app_entry)(const struct app_descriptor *, void *args);

/* app startup flags */
#define APP_FLAG_DONT_START_ON_BOOT 0x1

/* each app needs to define one of these to define its startup conditions */
struct app_descriptor {
	const char *name;
	app_init  init;
	app_entry entry;
	unsigned int flags;
};

typedef struct disk_partition {
	char part_name[16];
	char uuid[37];
}disk_partition_t;

#define UUID_BIN_LEN 16
#define MAX_BOOT_ARGS_SIZE 128
#define GPT_ENTRY_NUMBERS 128

int update_uuid(char *bootargs);
int set_uuid_bootargs(char *boot_args, char *part_name,
		int buflen, bool gpt_flag);
int get_partition_info_efi_by_name(const char *name, disk_partition_t *info);

#define APP_START(appname) struct app_descriptor _app_##appname __SECTION(".apps") = { .name = #appname,
#define APP_END };

#endif

