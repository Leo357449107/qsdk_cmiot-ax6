/*
 * Copyright (c) 2014 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <spi.h>
#include <spi_flash.h>

UCLASS_DRIVER(spi_emul) = {
	.id		= UCLASS_SPI_EMUL,
	.name		= "spi_emul",
};
