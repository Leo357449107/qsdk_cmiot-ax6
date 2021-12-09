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
#include <err.h>
#include <debug.h>
#include <platform.h>

/*
 * default implementations of these routines, if the platform code
 * chooses not to implement.
 */
__WEAK int platform_use_identity_mmu_mappings(void)
{
	return 1;
}

__WEAK void platform_init_mmu_mappings(void)
{
}

__WEAK addr_t platform_get_virt_to_phys_mapping(addr_t virt_addr)
{
	if (!(platform_use_identity_mmu_mappings())) {
		dprintf(CRITICAL,
			"%s: ERROR: platform_use_identity_mmu_mappings failed\n",
			__func__);
		return 0;
	}

	return virt_addr;
}

__WEAK addr_t platform_get_phys_to_virt_mapping(addr_t phys_addr)
{
	if (!(platform_use_identity_mmu_mappings())) {
		dprintf(CRITICAL,
                        "%s: ERROR: platform_use_identity_mmu_mappings failed\n",
                        __func__);
                return 0;
        }

	return phys_addr;
}

__WEAK void platform_early_init(void)
{
}

__WEAK void platform_init(void)
{
}

__WEAK void display_init(void)
{
}

__WEAK void display_shutdown(void)
{
}

__WEAK void platform_config_interleaved_mode_gpios(void)
{
}

__WEAK void uart_clock_init(void)
{
}

__WEAK void platform_uninit(void)
{
}

__WEAK int image_verify(unsigned char * image_ptr,
			unsigned char * signature_ptr,
			unsigned int image_size,
			unsigned hash_type)
{
	return 0;
}

__WEAK void ce_clock_init(void)
{
}
