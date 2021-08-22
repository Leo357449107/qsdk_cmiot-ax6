/*
 * Copyright (c) 2012-2013, 2015-2017, 2020, The Linux Foundation. All rights reserved.
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

#ifndef __UART_DM_H__
#define __UART_DM_H__

#include <stdio.h>

int uart_init(unsigned long base_addr);
int uart_putc(const char ch);

#define UART_DM_EXTR_BITS(value, start_pos, end_pos) \
                                             ((value << (32 - end_pos))\
                                              >> (32 - (end_pos - start_pos)))

#define PACK_CHARS_INTO_WORDS(a, cnt, word)  {                                 \
                                               word = 0;                       \
                                               int j;                          \
                                               for(j=0; j < (int)cnt; j++) { \
                                                   word |= (a[j] & 0xff)<< (j * 8);\
                                               }                               \
                                              }

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define PERIPH_BLK_BLSP 1

extern void __udelay(unsigned long usec);
void ipq_serial_wait_tx_empty(void);

enum UART_DM_PARITY_MODE {
        UART_DM_NO_PARITY,
        UART_DM_ODD_PARITY,
        UART_DM_EVEN_PARITY,
        UART_DM_SPACE_PARITY
};

/* UART Stop Bit Length */
enum UART_DM_STOP_BIT_LEN {
        UART_DM_SBL_9_16,
        UART_DM_SBL_1,
        UART_DM_SBL_1_9_16,
        UART_DM_SBL_2
};

/* UART Bits per Char */
enum UART_DM_BITS_PER_CHAR {
        UART_DM_5_BPS,
        UART_DM_6_BPS,
        UART_DM_7_BPS,
        UART_DM_8_BPS
};

/* 8-N-1 Configuration */
#define UART_DM_8_N_1_MODE          (UART_DM_NO_PARITY | \
                                             (UART_DM_SBL_1 << 2) | \
                                             (UART_DM_8_BPS << 4))

/* UART_DM Registers */

/* UART Operational Mode Register */
#define UART_DM_MR1(base)             ((base) + 0x00)
#define UART_DM_MR2(base)             ((base) + 0x04)
#define UART_DM_RXBRK_ZERO_CHAR_OFF (1 << 8)
#define UART_DM_LOOPBACK            (1 << 7)

/* UART Clock Selection Register */
#if PERIPH_BLK_BLSP
#define UART_DM_CSR(base)             ((base) + 0xA0)
#else
#define UART_DM_CSR(base)             ((base) + 0x08)
#endif

/* UART DM TX FIFO Registers - 4 */
#if PERIPH_BLK_BLSP
#define UART_DM_TF(base, x)         ((base) + 0x100+(4*(x)))
#else
#define UART_DM_TF(base, x)         ((base) + 0x70+(4*(x)))
#endif

/* UART Command Register */
#if PERIPH_BLK_BLSP
#define UART_DM_CR(base)              ((base) + 0xA8)
#else
#define UART_DM_CR(base)              ((base) + 0x10)
#endif
#define UART_DM_CR_RX_ENABLE        (1 << 0)
#define UART_DM_CR_RX_DISABLE       (1 << 1)
#define UART_DM_CR_TX_ENABLE        (1 << 2)
#define UART_DM_CR_TX_DISABLE       (1 << 3)

/* UART Channel Command */
#define UART_DM_CR_CH_CMD_LSB(x)    ((x & 0x0f) << 4)
#define UART_DM_CR_CH_CMD_MSB(x)    ((x >> 4 ) << 11 )
#define UART_DM_CR_CH_CMD(x)        (UART_DM_CR_CH_CMD_LSB(x)\
                                             | UART_DM_CR_CH_CMD_MSB(x))
#define UART_DM_CMD_NULL            UART_DM_CR_CH_CMD(0)
#define UART_DM_CMD_RESET_RX        UART_DM_CR_CH_CMD(1)
#define UART_DM_CMD_RESET_TX        UART_DM_CR_CH_CMD(2)
#define UART_DM_CMD_RESET_ERR_STAT  UART_DM_CR_CH_CMD(3)
#define UART_DM_CMD_RES_BRK_CHG_INT UART_DM_CR_CH_CMD(4)
#define UART_DM_CMD_START_BRK       UART_DM_CR_CH_CMD(5)
#define UART_DM_CMD_STOP_BRK        UART_DM_CR_CH_CMD(6)
#define UART_DM_CMD_RES_CTS_N       UART_DM_CR_CH_CMD(7)
#define UART_DM_CMD_RES_STALE_INT   UART_DM_CR_CH_CMD(8)
#define UART_DM_CMD_PACKET_MODE     UART_DM_CR_CH_CMD(9)
#define UART_DM_CMD_MODE_RESET      UART_DM_CR_CH_CMD(C)
#define UART_DM_CMD_SET_RFR_N       UART_DM_CR_CH_CMD(D)
#define UART_DM_CMD_RES_RFR_N       UART_DM_CR_CH_CMD(E)
#define UART_DM_CMD_RES_TX_ERR      UART_DM_CR_CH_CMD(10)
#define UART_DM_CMD_CLR_TX_DONE     UART_DM_CR_CH_CMD(11)
#define UART_DM_CMD_RES_BRKSTRT_INT UART_DM_CR_CH_CMD(12)
#define UART_DM_CMD_RES_BRKEND_INT  UART_DM_CR_CH_CMD(13)
#define UART_DM_CMD_RES_PER_FRM_INT UART_DM_CR_CH_CMD(14)

/*UART General Command */
#define UART_DM_CR_GENERAL_CMD(x)   ((x) << 8)

#define UART_DM_GCMD_NULL            UART_DM_CR_GENERAL_CMD(0)
#define UART_DM_GCMD_CR_PROT_EN      UART_DM_CR_GENERAL_CMD(1)
#define UART_DM_GCMD_CR_PROT_DIS     UART_DM_CR_GENERAL_CMD(2)
#define UART_DM_GCMD_RES_TX_RDY_INT  UART_DM_CR_GENERAL_CMD(3)
#define UART_DM_GCMD_SW_FORCE_STALE  UART_DM_CR_GENERAL_CMD(4)
#define UART_DM_GCMD_ENA_STALE_EVT   UART_DM_CR_GENERAL_CMD(5)
#define UART_DM_GCMD_DIS_STALE_EVT   UART_DM_CR_GENERAL_CMD(6)

/* UART Interrupt Mask Register */
#if PERIPH_BLK_BLSP
#define UART_DM_IMR(base)             ((base) + 0xB0)
#else
#define UART_DM_IMR(base)             ((base) + 0x14)
#endif

#define UART_DM_TXLEV               (1 << 0)
#define UART_DM_RXHUNT              (1 << 1)
#define UART_DM_RXBRK_CHNG          (1 << 2)
#define UART_DM_RXSTALE             (1 << 3)
#define UART_DM_RXLEV               (1 << 4)
#define UART_DM_DELTA_CTS           (1 << 5)
#define UART_DM_CURRENT_CTS         (1 << 6)
#define UART_DM_TX_READY            (1 << 7)
#define UART_DM_TX_ERROR            (1 << 8)
#define UART_DM_TX_DONE             (1 << 9)
#define UART_DM_RXBREAK_START       (1 << 10)
#define UART_DM_RXBREAK_END         (1 << 11)
#define UART_DM_PAR_FRAME_ERR_IRQ   (1 << 12)

#define UART_DM_IMR_ENABLED         (UART_DM_TX_READY | \
                                              UART_DM_TXLEV    | \
                                              UART_DM_RXSTALE)

/* UART Interrupt Programming Register */
#define UART_DM_IPR(base)             ((base) + 0x18)
#define UART_DM_STALE_TIMEOUT_LSB   0x0f
#define UART_DM_STALE_TIMEOUT_MSB   0	/* Not used currently */

/* UART Transmit/Receive FIFO Watermark Register */
#define UART_DM_TFWR(base)            ((base) + 0x1C)
/* Interrupt is generated when FIFO level is less than or equal to this value */
#define UART_DM_TFW_VALUE           0

#define UART_DM_RFWR(base)            ((base) + 0x20)
/*Interrupt generated when no of words in RX FIFO is greater than this value */
#define UART_DM_RFW_VALUE           0

/* UART Hunt Character Register */
#define UART_DM_HCR(base)             ((base) + 0x24)

/* Used for RX transfer initialization */
#define UART_DM_DMRX(base)            ((base) + 0x34)

/* Default DMRX value - any value bigger than FIFO size would be fine */
#define UART_DM_DMRX_DEF_VALUE    0x220

/* Register to enable IRDA function */
#if PERIPH_BLK_BLSP
#define UART_DM_IRDA(base)            ((base) + 0xB8)
#else
#define UART_DM_IRDA(base)            ((base) + 0x38)
#endif

/* UART Data Mover Enable Register */
#define UART_DM_DMEN(base)            ((base) + 0x3C)

/* Number of characters for Transmission */
#define UART_DM_NO_CHARS_FOR_TX(base) ((base) + 0x040)

/* UART RX FIFO Base Address */
#define UART_DM_BADR(base)            ((base) + 0x44)

/* UART Status Register */
#if PERIPH_BLK_BLSP
#define UART_DM_SR(base)              ((base) + 0x0A4)
#else
#define UART_DM_SR(base)              ((base) + 0x008)
#endif
#define UART_DM_SR_RXRDY            (1 << 0)
#define UART_DM_SR_RXFULL           (1 << 1)
#define UART_DM_SR_TXRDY            (1 << 2)
#define UART_DM_SR_TXEMT            (1 << 3)
#define UART_DM_SR_UART_OVERRUN     (1 << 4)
#define UART_DM_SR_PAR_FRAME_ERR    (1 << 5)
#define UART_DM_RX_BREAK            (1 << 6)
#define UART_DM_HUNT_CHAR           (1 << 7)
#define UART_DM_RX_BRK_START_LAST   (1 << 8)

/* UART Receive FIFO Registers - 4 in numbers */
#if PERIPH_BLK_BLSP
#define UART_DM_RF(base, x)      ((base) + 0x140 + (4*(x)))
#else
#define UART_DM_RF(base, x)      ((base) + 0x70 + (4*(x)))
#endif

/* UART Masked Interrupt Status Register */
#if PERIPH_BLK_BLSP
#define UART_DM_MISR(base)         ((base) + 0xAC)
#else
#define UART_DM_MISR(base)         ((base) + 0x10)
#endif

/* UART Interrupt Status Register */
#if PERIPH_BLK_BLSP
#define UART_DM_ISR(base)          ((base) + 0xB4)
#else
#define UART_DM_ISR(base)          ((base) + 0x14)
#endif

/* Number of characters received since the end of last RX transfer */
#if PERIPH_BLK_BLSP
#define UART_DM_RX_TOTAL_SNAP(base)  ((base) + 0xBC)
#else
#define UART_DM_RX_TOTAL_SNAP(base)  ((base) + 0x38)
#endif

/* UART TX FIFO Status Register */
#define UART_DM_TXFS(base)           ((base) + 0x4C)
#define UART_DM_TXFS_STATE_LSB(x)   UART_DM_EXTR_BITS(x,0,6)
#define UART_DM_TXFS_STATE_MSB(x)   UART_DM_EXTR_BITS(x,14,31)
#define UART_DM_TXFS_BUF_STATE(x)   UART_DM_EXTR_BITS(x,7,9)
#define UART_DM_TXFS_ASYNC_STATE(x) UART_DM_EXTR_BITS(x,10,13)

/* UART RX FIFO Status Register */
#define UART_DM_RXFS(base)           ((base) + 0x50)
#define UART_DM_RXFS_STATE_LSB(x)   UART_DM_EXTR_BITS(x,0,6)
#define UART_DM_RXFS_STATE_MSB(x)   UART_DM_EXTR_BITS(x,14,31)
#define UART_DM_RXFS_BUF_STATE(x)   UART_DM_EXTR_BITS(x,7,9)
#define UART_DM_RXFS_ASYNC_STATE(x) UART_DM_EXTR_BITS(x,10,13)

/* Macros for Common Errors */
#define UART_DM_E_SUCCESS           0
#define UART_DM_E_FAILURE           1
#define UART_DM_E_TIMEOUT           2
#define UART_DM_E_INVAL             3
#define UART_DM_E_MALLOC_FAIL       4
#define UART_DM_E_RX_NOT_READY      5

#endif				/* __UART_DM_H__ */
