/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 *
 * Updated-by: Prafulla Wadaskar <prafulla@marvell.com>
 *		default_image specific code abstracted from mkimage.c
 *		some functions added to address abstraction
 *
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "imagetool.h"
#include "mkimage.h"

#include <image.h>
#include <u-boot/crc.h>

static image_header_t header;

static int image_check_image_types(uint8_t type)
{
	if (((type > IH_TYPE_INVALID) && (type < IH_TYPE_FLATDT)) ||
	    (type == IH_TYPE_KERNEL_NOLOAD))
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int image_check_params(struct image_tool_params *params)
{
	return	((params->dflag && (params->fflag || params->lflag)) ||
		(params->fflag && (params->dflag || params->lflag)) ||
		(params->lflag && (params->dflag || params->fflag)));
}

static int image_verify_header(unsigned char *ptr, int image_size,
			struct image_tool_params *params)
{
	uint32_t len;
	const unsigned char *data;
	uint32_t checksum;
	image_header_t header;
	image_header_t *hdr = &header;

	/*
	 * create copy of header so that we can blank out the
	 * checksum field for checking - this can't be done
	 * on the PROT_READ mapped data.
	 */
	memcpy(hdr, ptr, sizeof(image_header_t));

	if (be32_to_cpu(hdr->ih_magic) != IH_MAGIC) {
		debug("%s: Bad Magic Number: \"%s\" is no valid image\n",
		      params->cmdname, params->imagefile);
		return -FDT_ERR_BADMAGIC;
	}

	data = (const unsigned char *)hdr;
	len  = sizeof(image_header_t);

	checksum = be32_to_cpu(hdr->ih_hcrc);
	hdr->ih_hcrc = cpu_to_be32(0);	/* clear for re-calculation */

	if (crc32(0, data, len) != checksum) {
		debug("%s: ERROR: \"%s\" has bad header checksum!\n",
		      params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTATE;
	}

	data = (const unsigned char *)ptr + sizeof(image_header_t);
	len  = image_size - sizeof(image_header_t) ;

	checksum = be32_to_cpu(hdr->ih_dcrc);
	if (crc32(0, data, len) != checksum) {
		debug("%s: ERROR: \"%s\" has corrupted data!\n",
		      params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTRUCTURE;
	}
	return 0;
}

static void image_set_header(void *ptr, struct stat *sbuf, int ifd,
				struct image_tool_params *params)
{
	uint32_t checksum;
	char *source_date_epoch;
	time_t time;

	image_header_t * hdr = (image_header_t *)ptr;

	checksum = crc32(0,
			(const unsigned char *)(ptr +
				sizeof(image_header_t)),
			sbuf->st_size - sizeof(image_header_t));

	source_date_epoch = getenv("SOURCE_DATE_EPOCH");
	if (source_date_epoch != NULL) {
		time = (time_t) strtol(source_date_epoch, NULL, 10);

		if (gmtime(&time) == NULL) {
			fprintf(stderr, "%s: SOURCE_DATE_EPOCH is not valid\n",
				__func__);
			time = 0;
		}
	} else {
		time = sbuf->st_mtime;
	}

	/* Build new header */
	image_set_magic(hdr, IH_MAGIC);
	image_set_time(hdr, time);
	image_set_size(hdr, sbuf->st_size - sizeof(image_header_t));
	image_set_load(hdr, params->addr);
	image_set_ep(hdr, params->ep);
	image_set_dcrc(hdr, checksum);
	image_set_os(hdr, params->os);
	image_set_arch(hdr, params->arch);
	image_set_type(hdr, params->type);
	image_set_comp(hdr, params->comp);

	image_set_name(hdr, params->imagename);

	checksum = crc32(0, (const unsigned char *)hdr,
				sizeof(image_header_t));

	image_set_hcrc(hdr, checksum);
}

static int image_extract_subimage(void *ptr, struct image_tool_params *params)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	ulong file_data;
	ulong file_len;

	if (image_check_type(hdr, IH_TYPE_MULTI)) {
		ulong idx = params->pflag;
		ulong count;

		/* get the number of data files present in the image */
		count = image_multi_count(hdr);

		/* retrieve the "data file" at the idx position */
		image_multi_getimg(hdr, idx, &file_data, &file_len);

		if ((file_len == 0) || (idx >= count)) {
			fprintf(stderr, "%s: No such data file %ld in \"%s\"\n",
				params->cmdname, idx, params->imagefile);
			return -1;
		}
	} else {
		file_data = image_get_data(hdr);
		file_len = image_get_size(hdr);
	}

	/* save the "data file" into the file system */
	return imagetool_save_subimage(params->outfile, file_data, file_len);
}

/*
 * Default image type parameters definition
 */
U_BOOT_IMAGE_TYPE(
	defimage,
	"Default Image support",
	sizeof(image_header_t),
	(void *)&header,
	image_check_params,
	image_verify_header,
	image_print_contents,
	image_set_header,
	image_extract_subimage,
	image_check_image_types,
	NULL,
	NULL
);
