/*
 * Copyright (c) 2012-2017, 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 *  * Neither the name of [Owner Organization] nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "uart.h"
#include <lib/mmio.h>
#include <stdint.h>

/**
 * uart_dm_init_rx_transfer - Init Rx transfer
 * @uart_dm_base: UART controller base address
 */
static unsigned int
uart_dm_init_rx_transfer(unsigned long uart_dm_base)
{
	/* Reset receiver */
	mmio_write_32(UART_DM_CR(uart_dm_base), UART_DM_CMD_RESET_RX);

	/* Enable receiver */
	mmio_write_32(UART_DM_CR(uart_dm_base), UART_DM_CR_RX_ENABLE);
	mmio_write_32(UART_DM_DMRX(uart_dm_base), UART_DM_DMRX_DEF_VALUE);

	/* Clear stale event */
	mmio_write_32(UART_DM_CR(uart_dm_base), UART_DM_CMD_RES_STALE_INT);

	/* Enable stale event */
	mmio_write_32(UART_DM_CR(uart_dm_base), UART_DM_GCMD_ENA_STALE_EVT);

	return UART_DM_E_SUCCESS;
}

/*
 * uart_dm_reset - resets UART controller
 * @base: UART controller base address
 */
static unsigned int uart_dm_reset(unsigned long base)
{
	mmio_write_32(UART_DM_CR(base), UART_DM_CMD_RESET_RX);
	mmio_write_32(UART_DM_CR(base), UART_DM_CMD_RESET_TX);
	mmio_write_32(UART_DM_CR(base), UART_DM_CMD_RESET_ERR_STAT);
	mmio_write_32(UART_DM_CR(base), UART_DM_CMD_RES_TX_ERR);
	mmio_write_32(UART_DM_CR(base), UART_DM_CMD_RES_STALE_INT);

	return UART_DM_E_SUCCESS;
}

/*
 * uart_dm_init - initilaizes UART controller
 * @uart_dm_base: UART controller base address
 */
static unsigned int uart_dm_init(unsigned long uart_dm_base)
{
	/* Configure UART mode registers MR1 and MR2 */
	/* Hardware flow control isn't supported */
	mmio_write_32(UART_DM_MR1(uart_dm_base), 0x0);

	/* 8-N-1 configuration: 8 data bits - No parity - 1 stop bit */
	mmio_write_32(UART_DM_MR2(uart_dm_base), UART_DM_8_N_1_MODE);

	/* Configure Interrupt Mask register IMR */
	mmio_write_32(UART_DM_IMR(uart_dm_base), UART_DM_IMR_ENABLED);

	/*
	 * Configure Tx and Rx watermarks configuration registers
	 * TX watermark value is set to 0 - interrupt is generated when
	 * FIFO level is less than or equal to 0
	 */
	mmio_write_32(UART_DM_TFWR(uart_dm_base), UART_DM_TFW_VALUE);

	/* RX watermark value */
	mmio_write_32(UART_DM_RFWR(uart_dm_base), UART_DM_RFW_VALUE);

	/* Configure Interrupt Programming Register */
	/* Set initial Stale timeout value */
	mmio_write_32(UART_DM_IPR(uart_dm_base), UART_DM_STALE_TIMEOUT_LSB);

	/* Configure IRDA if required */
	/* Disabling IRDA mode */
	mmio_write_32(UART_DM_IRDA(uart_dm_base), 0x0);

	/* Configure hunt character value in HCR register */
	/* Keep it in reset state */
	mmio_write_32(UART_DM_HCR(uart_dm_base), 0x0);

	/*
	 * Configure Rx FIFO base address
	 * Both TX/RX shares same SRAM and default is half-n-half.
	 * Sticking with default value now.
	 * As such RAM size is (2^RAM_ADDR_WIDTH, 32-bit entries).
	 * We have found RAM_ADDR_WIDTH = 0x7f
	 */

	/* Issue soft reset command */
	uart_dm_reset(uart_dm_base);

	/* Enable/Disable Rx/Tx DM interfaces */
	/* Data Mover not currently utilized. */
	mmio_write_32(UART_DM_DMEN(uart_dm_base), 0x0);

	/* Enable transmitter */
	mmio_write_32(UART_DM_CR(uart_dm_base), UART_DM_CR_TX_ENABLE);

	/* Initialize Receive Path */
	uart_dm_init_rx_transfer(uart_dm_base);

	return 0;
}

/**
 * uart_replace_lr_with_cr - replaces "\n" with "\r\n"
 * @data_in:      characters to be converted
 * @num_of_chars: no. of characters
 * @data_out:     location where converted chars are stored
 *
 * Replace linefeed char "\n" with carriage return + linefeed
 * "\r\n". Currently keeping it simple than efficient.
 */
static unsigned int
uart_replace_lr_with_cr(const char *data_in,
                                 int num_of_chars,
                                 char *data_out, int *num_of_chars_out)
{
        int i = 0, j = 0;

        if ((data_in == NULL) || (data_out == NULL) || (num_of_chars < 0))
                return UART_DM_E_INVAL;

        for (i = 0, j = 0; i < num_of_chars; i++, j++) {
                if (data_in[i] == '\n')
                        data_out[j++] = '\r';

                data_out[j] = data_in[i];
        }

        *num_of_chars_out = j;

        return UART_DM_E_SUCCESS;
}

/**
 * uart_dm_write - transmit data
 * @data:          data to transmit
 * @num_of_chars:  no. of bytes to transmit
 *
 * Writes the data to the TX FIFO. If no space is available blocks
 * till space becomes available.
 */
static unsigned int
uart_dm_write(const char *data, unsigned int num_of_chars,
			unsigned long base)
{
	unsigned int tx_word_count = 0;
	unsigned int tx_char_left = 0, tx_char = 0;
	unsigned int tx_word = 0;
	int i = 0;
	char *tx_data = NULL;
	char new_data[1024];

        if ((data == NULL) || (num_of_chars <= 0))
                return UART_DM_E_INVAL;

        /* Replace line-feed (/n) with carriage-return + line-feed (/r/n) */
        uart_replace_lr_with_cr(data, num_of_chars, new_data, &i);

        tx_data = new_data;
        num_of_chars = i;

        /* Write to NO_CHARS_FOR_TX register number of characters
        * to be transmitted. However, before writing TX_FIFO must
        * be empty as indicated by TX_READY interrupt in IMR register
        */
        /* Check if transmit FIFO is empty.
        * If not we'll wait for TX_READY interrupt. */

        if (!(mmio_read_32(UART_DM_SR(base)) & UART_DM_SR_TXEMT)) {
                while (!(mmio_read_32(UART_DM_ISR(base)) & UART_DM_TX_READY))
				{

				}
        }

        /* We are here. FIFO is ready to be written. */
        /* Write number of characters to be written */
        mmio_write_32(UART_DM_NO_CHARS_FOR_TX(base), num_of_chars);

        /* Clear TX_READY interrupt */
        mmio_write_32(UART_DM_CR(base), UART_DM_GCMD_RES_TX_RDY_INT);

        /* We use four-character word FIFO. So we need to divide data into
        * four characters and write in UART_DM_TF register */
        tx_word_count = (num_of_chars % 4) ? ((num_of_chars / 4) + 1) :
                        (num_of_chars / 4);
        tx_char_left = num_of_chars;

        for (i = 0; i < (int)tx_word_count; i++) {
                tx_char = (tx_char_left < 4) ? tx_char_left : 4;
                PACK_CHARS_INTO_WORDS(tx_data, tx_char, tx_word);

                /* Wait till TX FIFO has space */
                while (!(mmio_read_32(UART_DM_SR(base)) & UART_DM_SR_TXRDY))
				{

				}

                /* TX FIFO has space. Write the chars */
                mmio_write_32(UART_DM_TF(base, 0), tx_word);
                tx_char_left = num_of_chars - (i + 1) * 4;
                tx_data = tx_data + 4;
        }

        return UART_DM_E_SUCCESS;
}

unsigned long uart_base = 0;

int uart_putc(const char ch)
	{
		return uart_dm_write(&ch, 1, uart_base);
	}

int uart_init(unsigned long base_addr)
	 {
		 uart_base = base_addr;
		 uart_dm_init(base_addr);
		 return 0;
	 }
