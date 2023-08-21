#include "audio_out.h"
#include "muter.h"

#include "loggers.h"

#include <I2S.h>

#include <byte_swap_tx.h>
#include <byte_swap_tx_defs.h>

#include <DMA_AudioTxProcess_dma.h>
#include <DMA_BS_RX_dma.h>
#include <DMA_BS_TX_dma.h>
#include <DMA_I2S_TX_dma.h>

#include <process_tx_isr.h>
#include <i2s_tx_isr.h>
#include <bs_tx_isr.h>

#include <stdlib.h>

#define DMA_MAX_TRANSFER_SIZE 4095

#define FIFO_HALF_FULL_MASK 0x0C

volatile int audio_out_buffer_size = 0;
volatile uint8_t audio_out_status = 0;

#define MAX_TDS 32

// Transfer audio memory source to its destination.
// The destination may be the byte swap component or the transmit buffer.
//
// When using this with usb, call audio_out_transmit on the data buffer from the usb endpoint
//
// Another option is to process the usb buffer in code (and perform the endian swap) to skip the byte swap.
// then transfer the data to the transmit buffer.
//
static uint8_t source_dma_ch;
static uint8_t source_dma_td[1];

// Optional byte-swap operation to swap the endianness of 24 bit data.
// Transfers data from memory to the byte-swap component,
// Then the endian-swapped bytes are transferred to the transmit buffer.
static uint8_t bs_dma_ch;
static uint8_t bs_dma_td[MAX_TDS];

// I2S Transmit dma. Transfers data from the transmit buffer to the i2s component.
// dma tds transfers of equal sizes will be chained and there will be an interrupt for each transfer.
// This interrupt gives an update on the buffer size.
/*
    This will infinitely loop and send the transmit buffer.
*/
static uint8_t i2s_dma_ch;
static uint8_t i2s_dma_td[MAX_TDS];

// Configure byte swap dma
static void bs_dma_config(void);
// Configure i2s dma
static void i2s_dma_config(void);

// ISRs for TopDesign peripherals.
CY_ISR_PROTO(source_done_isr);
CY_ISR_PROTO(i2s_tx_done_isr);

static const audio_out_config *config;

static int active_limit = 0;
static int underflow_limit = 0;
static int overflow_limit = 0;

static uint16_t source_dst_addr = 0;
static uint8_t source_td_config = 0;

static int buffer_capacity = 0;

void audio_out_init(const audio_out_config *_config)
{
    config = _config;

    buffer_capacity = config->transfer_size * config->n_tds;

    // Turn on I2S when buffer is half full.
    active_limit = buffer_capacity >> 1;

    underflow_limit = config->transfer_size * 2;
    overflow_limit = buffer_capacity - underflow_limit;

    // Send usb data to another source.
    source_dma_td[0] = CyDmaTdAllocate();

    // Use the byte swap component
    if (config->swap_endian)
    {
        // Send source data to the byte swap component
        source_dma_ch = DMA_BS_RX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
        source_dst_addr = LO16((uint32_t)byte_swap_tx_fifo_in_ptr);
        source_td_config = TD_INC_SRC_ADR;

        bs_dma_config();
        bs_tx_isr_StartEx(source_done_isr);
    }
    else
    {
        // Send source data directly to tx_buffer.
        source_dma_ch = DMA_AudioTxProcess_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
        source_dst_addr = LO16((uint32_t)config->buffer);
        source_td_config = TD_INC_SRC_ADR | TD_INC_DST_ADR | DMA_AudioTxProcess__TD_TERMOUT_EN;

        process_tx_isr_StartEx(source_done_isr);
    }

    // Initialize the i2s modules
    i2s_dma_config();
    i2s_tx_isr_StartEx(i2s_tx_done_isr);

    I2S_Start();

    // Initialize buffer management.
    audio_out_buffer_size = 0;
    audio_out_status = 0;

    // Set UDB fifos to use half full flags.
    byte_swap_tx_DP_F0_SET_LEVEL_MID;
    byte_swap_tx_DP_F1_SET_LEVEL_MID;
    I2S_TX_AUX_CONTROL_REG = I2S_TX_AUX_CONTROL_REG | FIFO_HALF_FULL_MASK;
}

void audio_out_transmit(const uint8_t *audio_buf, uint16_t amount)
{
    uint8_t int_status = 0;

    if (audio_out_buffer_size >= overflow_limit)
    {
        audio_out_status |= AUDIO_OUT_STS_OVERFLOW;
        log_error(&serial_log, "Overflow");
        // Drop data on the floor? Pause USB?
        return;
    }

    // Start a dma transaction to send data to the destination
    CyDmaTdSetConfiguration(source_dma_td[0], amount, CY_DMA_DISABLE_TD, source_td_config);
    CyDmaTdSetAddress(source_dma_td[0], LO16((uint32_t)audio_buf), source_dst_addr);
    CyDmaChSetInitialTd(source_dma_ch, source_dma_td[0]);
    CyDmaChEnable(source_dma_ch, 1u);

    // If we're copying bytes directly, it may be more/fewer than config->transfer_size
    audio_out_buffer_size += amount;
    // If it's off and we're over half full, start it up.
    if (audio_out_buffer_size >= active_limit)
    {
        audio_out_enable();
    }
}

static void bs_dma_config(void)
{
    // Send byte swapped data to the tx buffer.
    bs_dma_ch = DMA_BS_TX_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));

    uint8_t td_config = TD_INC_DST_ADR | DMA_BS_TX__TD_TERMOUT_EN;
    const uint8_t *dst = config->buffer;
    
    // Use the max transfer size for byte swap transfers.
    int remaining = buffer_capacity;
    uint8_t *td = bs_dma_td;
    int required_tds = (remaining + DMA_MAX_TRANSFER_SIZE - 1) / DMA_MAX_TRANSFER_SIZE;
    for (int i = 0; i < required_tds; i++)
    {
        bs_dma_td[i] = CyDmaTdAllocate();
    }
    
    while (remaining > 0)
    {
        int transfer_size = remaining > DMA_MAX_TRANSFER_SIZE ? DMA_MAX_TRANSFER_SIZE : remaining;
        remaining -= transfer_size;
        
        uint8_t next_td = remaining == 0 ? bs_dma_td[0] : td[1];
        
        CyDmaTdSetConfiguration(*td, transfer_size, next_td, td_config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)byte_swap_tx_fifo_out_ptr), LO16((uint32_t)dst));
        
        dst += transfer_size;
        td++;
    }
    
    // Use the i2s transfer size for byte swap transfers.
//    for (int i = 0; i < config->n_tds; i++)
//    {
//        bs_dma_td[i] = CyDmaTdAllocate();
//    }
//    for (int i = 0; i < config->n_tds; i++)
//    {
//        CyDmaTdSetConfiguration(bs_dma_td[i], config->transfer_size, bs_dma_td[(i + 1) % config->n_tds], td_config);
//        CyDmaTdSetAddress(bs_dma_td[i], LO16((uint32_t)byte_swap_tx_fifo_out_ptr), LO16((uint32_t)dst));
//        dst += config->transfer_size;
//    }

    CyDmaChSetInitialTd(bs_dma_ch, bs_dma_td[0]);
    CyDmaChEnable(bs_dma_ch, 1u);
}

static void i2s_dma_config(void)
{
    i2s_dma_ch = DMA_I2S_TX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    for (int i = 0; i < config->n_tds; i++)
    {
        i2s_dma_td[i] = CyDmaTdAllocate();
    }

    uint8_t td_config = TD_INC_SRC_ADR | DMA_I2S_TX__TD_TERMOUT_EN;
    const uint8_t *src = config->buffer;

    // Chain tds. We get an interrupt every config->transfer_size bytes.
    for (int i = 0; i < config->n_tds; i++)
    {
        CyDmaTdSetConfiguration(i2s_dma_td[i], config->transfer_size, i2s_dma_td[(i + 1) % config->n_tds], td_config);
        CyDmaTdSetAddress(i2s_dma_td[i], LO16((uint32_t)src), LO16((uint32_t)I2S_TX_CH0_F0_PTR));
        src += config->transfer_size;
    }

    CyDmaChSetInitialTd(i2s_dma_ch, i2s_dma_td[0]);
    CyDmaChEnable(i2s_dma_ch, 1u);
}

// Byte swap transfer completed.
CY_ISR(source_done_isr)
{
    // audio_out_buffer_size += config->transfer_size;
}

/* This isr is when each I2S transfer completes. This will happen
 * in regular increments of AUDIO_OUT_TRANSFER_SIZE. So we can
 * decrease the buffer size by that much.
 */
CY_ISR(i2s_tx_done_isr)
{
    audio_out_buffer_size -= config->transfer_size;
    // About to underflow. Oh nooooo.
    if (audio_out_buffer_size <= underflow_limit && (audio_out_status && AUDIO_OUT_STS_ACTIVE))
    {
        // What do?
        audio_out_status |= AUDIO_OUT_STS_UNDERFLOW;
        audio_out_status |= AUDIO_OUT_STS_RESET;
        audio_out_disable();
        log_error(&serial_log, "Underflow");
    }
}
