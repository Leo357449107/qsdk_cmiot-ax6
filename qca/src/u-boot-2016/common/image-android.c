/*
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <errno.h>

#define ANDROID_IMAGE_DEFAULT_KERNEL_ADDR	0x10008000

static char andr_tmp_str[ANDR_BOOT_ARGS_SIZE + 1];

static ulong android_image_get_kernel_addr(const struct andr_img_hdr *hdr)
{
	/*
	 * All the Android tools that generate a boot.img use this
	 * address as the default.
	 *
	 * Even though it doesn't really make a lot of sense, and it
	 * might be valid on some platforms, we treat that adress as
	 * the default value for this field, and try to execute the
	 * kernel in place in such a case.
	 *
	 * Otherwise, we will return the actual value set by the user.
	 */
	if (hdr->kernel_addr == ANDROID_IMAGE_DEFAULT_KERNEL_ADDR)
		return (ulong)hdr + hdr->page_size;

	return hdr->kernel_addr;
}

/**
 * android_image_get_kernel() - processes kernel part of Android boot images
 * @hdr:	Pointer to image header, which is at the start
 *			of the image.
 * @verify:	Checksum verification flag. Currently unimplemented.
 * @os_data:	Pointer to a ulong variable, will hold os data start
 *			address.
 * @os_len:	Pointer to a ulong variable, will hold os data length.
 *
 * This function returns the os image's start address and length. Also,
 * it appends the kernel command line to the bootargs env variable.
 *
 * Return: Zero, os start address and length on success,
 *		otherwise on failure.
 */
int android_image_get_kernel(const struct andr_img_hdr *hdr, int verify,
			     ulong *os_data, ulong *os_len)
{
	u32 kernel_addr = android_image_get_kernel_addr(hdr);

	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
	strncpy(andr_tmp_str, hdr->name, ANDR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	printf("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(hdr->kernel_size, 1024));

	int len = 0;
	if (*hdr->cmdline) {
		printf("Kernel command line: %s\n", hdr->cmdline);
		len += strlen(hdr->cmdline);
	}

	char *bootargs = getenv("bootargs");
	if (bootargs)
		len += strlen(bootargs);

	char *newbootargs = malloc(len + 2);
	if (!newbootargs) {
		puts("Error: malloc in android_image_get_kernel failed!\n");
		return -ENOMEM;
	}
	*newbootargs = '\0';

	if (bootargs) {
		strcpy(newbootargs, bootargs);
		strcat(newbootargs, " ");
	}
	if (*hdr->cmdline)
		strcat(newbootargs, hdr->cmdline);

	setenv("bootargs", newbootargs);

	if (os_data) {
		*os_data = (ulong)hdr;
		*os_data += hdr->page_size;
	}
	if (os_len)
		*os_len = hdr->kernel_size;
	return 0;
}

int android_image_check_header(const struct andr_img_hdr *hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE);
}

ulong android_image_get_end(const struct andr_img_hdr *hdr)
{
	ulong end;
	/*
	 * The header takes a full page, the remaining components are aligned
	 * on page boundary
	 */
	end = (ulong)hdr;
	end += hdr->page_size;
	end += ALIGN(hdr->kernel_size, hdr->page_size);
	end += ALIGN(hdr->ramdisk_size, hdr->page_size);
	end += ALIGN(hdr->second_size, hdr->page_size);

	return end;
}

ulong android_image_get_kload(const struct andr_img_hdr *hdr)
{
	return android_image_get_kernel_addr(hdr);
}

int android_image_get_ramdisk(const struct andr_img_hdr *hdr,
			      ulong *rd_data, ulong *rd_len)
{
	if (!hdr->ramdisk_size) {
		*rd_data = *rd_len = 0;
		return -1;
	}

	printf("RAM disk load addr 0x%08x size %u KiB\n",
	       hdr->ramdisk_addr, DIV_ROUND_UP(hdr->ramdisk_size, 1024));

	*rd_data = (unsigned long)hdr;
	*rd_data += hdr->page_size;
	*rd_data += ALIGN(hdr->kernel_size, hdr->page_size);

	*rd_len = hdr->ramdisk_size;
	return 0;
}
