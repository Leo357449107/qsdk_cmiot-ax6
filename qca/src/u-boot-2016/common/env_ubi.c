/*
 * (c) Copyright 2012 by National Instruments,
 *        Joe Hershberger <joe.hershberger@ni.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <command.h>
#include <environment.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <search.h>
#include <ubi_uboot.h>
#undef crc32

char *env_name_spec = "UBI";

env_t *env_ptr;

DECLARE_GLOBAL_DATA_PTR;

int env_init(void)
{
	/* use default */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}

#ifdef CONFIG_CMD_SAVEENV
#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
static unsigned char env_flags;

int saveenv(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	int ret;

	ret = env_export(env_new);
	if (ret)
		return ret;

	if (ubi_part(CONFIG_ENV_UBI_PART, NULL)) {
		printf("\n** Cannot find mtd partition \"%s\"\n",
		       CONFIG_ENV_UBI_PART);
		return 1;
	}

	env_new->flags = ++env_flags; /* increase the serial */

	if (gd->env_valid == 1) {
		puts("Writing to redundant UBI... ");
		if (ubi_volume_write(CONFIG_ENV_UBI_VOLUME_REDUND,
				     (void *)env_new, CONFIG_ENV_SIZE)) {
			printf("\n** Unable to write env to %s:%s **\n",
			       CONFIG_ENV_UBI_PART,
			       CONFIG_ENV_UBI_VOLUME_REDUND);
			return 1;
		}
	} else {
		puts("Writing to UBI... ");
		if (ubi_volume_write(CONFIG_ENV_UBI_VOLUME,
				     (void *)env_new, CONFIG_ENV_SIZE)) {
			printf("\n** Unable to write env to %s:%s **\n",
			       CONFIG_ENV_UBI_PART,
			       CONFIG_ENV_UBI_VOLUME);
			return 1;
		}
	}

	puts("done\n");

	gd->env_valid = gd->env_valid == 2 ? 1 : 2;

	return 0;
}
#else /* ! CONFIG_SYS_REDUNDAND_ENVIRONMENT */
int saveenv(void)
{
	int ret;
	env_t *env_new = (env_t *)memalign(ARCH_DMA_MINALIGN, CONFIG_ENV_SIZE)
	if (!env_new) {
		printf("Error: Cannot allocate %d bytes\n", CONFIG_ENV_SIZE);
		return 1;
        }

	ret = env_export(env_new);
	if (ret) {
		free(env_new);
		return ret;
	}

	if (ubi_part(CONFIG_ENV_UBI_PART, NULL)) {
		free(env_new);
		printf("\n** Cannot find mtd partition \"%s\"\n",
		       CONFIG_ENV_UBI_PART);
		return 1;
	}

	if (ubi_volume_write(CONFIG_ENV_UBI_VOLUME, (void *)env_new,
			     CONFIG_ENV_SIZE)) {
		free(env_new);
		printf("\n** Unable to write env to %s:%s **\n",
		       CONFIG_ENV_UBI_PART, CONFIG_ENV_UBI_VOLUME);
		return 1;
	}

	free(env_new);
	puts("done\n");
	return 0;
}
#endif /* CONFIG_SYS_REDUNDAND_ENVIRONMENT */
#endif /* CONFIG_CMD_SAVEENV */

#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
void env_relocate_spec(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, env1_buf, CONFIG_ENV_SIZE);
	ALLOC_CACHE_ALIGN_BUFFER(char, env2_buf, CONFIG_ENV_SIZE);
	int crc1_ok = 0, crc2_ok = 0;
	env_t *ep, *tmp_env1, *tmp_env2;

	tmp_env1 = (env_t *)env1_buf;
	tmp_env2 = (env_t *)env2_buf;

	if (ubi_part(CONFIG_ENV_UBI_PART, NULL)) {
		printf("\n** Cannot find mtd partition \"%s\"\n",
		       CONFIG_ENV_UBI_PART);
		set_default_env(NULL);
		return;
	}

	if (ubi_volume_read(CONFIG_ENV_UBI_VOLUME, (void *)tmp_env1,
			    CONFIG_ENV_SIZE)) {
		printf("\n** Unable to read env from %s:%s **\n",
		       CONFIG_ENV_UBI_PART, CONFIG_ENV_UBI_VOLUME);
	}

	if (ubi_volume_read(CONFIG_ENV_UBI_VOLUME_REDUND, (void *)tmp_env2,
			    CONFIG_ENV_SIZE)) {
		printf("\n** Unable to read redundant env from %s:%s **\n",
		       CONFIG_ENV_UBI_PART, CONFIG_ENV_UBI_VOLUME_REDUND);
	}

	crc1_ok = crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc;
	crc2_ok = crc32(0, tmp_env2->data, ENV_SIZE) == tmp_env2->crc;

	if (!crc1_ok && !crc2_ok) {
		set_default_env("!bad CRC");
		return;
	} else if (crc1_ok && !crc2_ok) {
		gd->env_valid = 1;
	} else if (!crc1_ok && crc2_ok) {
		gd->env_valid = 2;
	} else {
		/* both ok - check serial */
		if (tmp_env1->flags == 255 && tmp_env2->flags == 0)
			gd->env_valid = 2;
		else if (tmp_env2->flags == 255 && tmp_env1->flags == 0)
			gd->env_valid = 1;
		else if (tmp_env1->flags > tmp_env2->flags)
			gd->env_valid = 1;
		else if (tmp_env2->flags > tmp_env1->flags)
			gd->env_valid = 2;
		else /* flags are equal - almost impossible */
			gd->env_valid = 1;
	}

	if (gd->env_valid == 1)
		ep = tmp_env1;
	else
		ep = tmp_env2;

	env_flags = ep->flags;
	env_import((char *)ep, 0);
}
#else /* ! CONFIG_SYS_REDUNDAND_ENVIRONMENT */
void env_relocate_spec(void)
{

	char *buf = NULL;
	buf = (char *)memalign(ARCH_DMA_MINALIGN, CONFIG_ENV_SIZE);
	if (!buf) {
		printf("Error: Cannot allocate %d bytes\n");
		set_default_env(NULL);
		return;
	}

	if (ubi_part(CONFIG_ENV_UBI_PART, NULL)) {
		printf("\n** Cannot find mtd partition \"%s\"\n",
		       CONFIG_ENV_UBI_PART);
		set_default_env(NULL);
		free(buf);
		return;
	}

	if (ubi_volume_read(CONFIG_ENV_UBI_VOLUME, buf, CONFIG_ENV_SIZE)) {
		printf("\n** Unable to read env from %s:%s **\n",
		       CONFIG_ENV_UBI_PART, CONFIG_ENV_UBI_VOLUME);
		set_default_env(NULL);
		free(buf);
		return;
	}

	env_import(buf, 1);
	free(buf);
}
#endif /* CONFIG_SYS_REDUNDAND_ENVIRONMENT */
