#include "audio_tx.h"
#include "loggers.h"
#include "project_config.h"

#include "serial.h"

#include <I2S.h>

#include <byte_swap_tx.h>
#include <byte_swap_tx_defs.h>

#include <DMA_BS_RX_dma.h>
#include <DMA_BS_TX_dma.h>
#include <DMA_I2S_TX_dma.h>

#include <i2s_tx_isr.h>

#include <stdlib.h>

#define DMA_MAX_TRANSFER_SIZE 4095
#define FIFO_HALF_FULL_MASK 0x0C

volatile int audio_out_buffer_size = 0;
uint8_t audio_out_status = 0;

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
#define MAX_BS_TDS 16 // enough for entire memory...

// I2S Transmit dma. Transfers data from the transmit buffer to the i2s component.
// dma tds transfers of equal sizes will be chained and there will be an interrupt for each transfer.
// This interrupt gives an update on the buffer size.
/*
    This will infinitely loop and send the transmit buffer.
*/


// Initiate a dma transaction
static void configure_source_dma(const uint8_t *src, int amount);
// Configure byte swap dma
static void bs_dma_config(void);
// Configure i2s dma
static void i2s_dma_config(void);

// ISRs for TopDesign peripherals.
CY_ISR_PROTO(i2s_tx_done_isr);

static uint8_t tx_buffer[AUDIO_TX_BUF_SIZE];
static TaskHandle_t xAudioTxMonitor = NULL;

int audio_tx_size(void)
{
    return audio_out_buffer_size;
}

// Configure and start up all components for audio
void audio_out_init(void)
{
    // Send source data to the byte swap component
    source_dma_ch = DMA_BS_RX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    source_dma_td[0] = CyDmaTdAllocate();

    // Configure byte swap to transfer data to the tx_buffer.
    bs_dma_config();

    // Initialize the i2s modules
    i2s_dma_config();
    i2s_tx_isr_StartEx(i2s_tx_done_isr);
    I2S_Start();
    I2S_DisableTx();

    // Initialize buffer management.
    audio_out_buffer_size = 0;
    audio_out_status = 0;

    // Set UDB fifos to use half full flags.
    byte_swap_tx_DP_F0_SET_LEVEL_MID;
    byte_swap_tx_DP_F1_SET_LEVEL_MID;
    I2S_TX_AUX_CONTROL_REG |= FIFO_HALF_FULL_MASK;
}

// Send audio data to the byte swap component, which then gets sent to the transmit buffer.
void AudioTransmit(void *source_buffer)
{
    int result = 0;
    uint32_t source_buffer_size = 0;
    const TickType_t xMaxWait = pdMS_TO_TICKS(1);
    
    for (ever)
    {
        result = xTaskNotifyWait(0, UINT32_MAX, &source_buffer_size, xMaxWait);
        if (result)
        {
            // If we're in an overflow state and above half full, drop any incoming data.
            if (audio_out_status & AUDIO_OUT_STS_OVERFLOW)
            {
                log_error(&serial_log, "Overflowed! Dropping Data!\n");
            }
            else
            {
                // Start a dma transaction to send data to the byte swap component
                configure_source_dma(source_buffer, source_buffer_size);
                audio_out_buffer_size += source_buffer_size;
            }
        }
    }
}

void AudioTxMonitor(void *pvParameters)
{
    (void)pvParameters;
    
    xAudioTxMonitor = xTaskGetCurrentTaskHandle();
    // Set based on the transfer size.
    const TickType_t xMaxWait = pdMS_TO_TICKS(1);

    uint32_t notifications = 0;
    
    for (ever)
    {
        // Process all outstanding notifications.
        notifications = ulTaskNotifyTake(pdTRUE, xMaxWait);
        if (notifications == 0)
        {
            // Timed out. I2S is not currently enabled.
            if (!(audio_out_status & AUDIO_OUT_STS_ACTIVE) && audio_out_buffer_size >= AUDIO_TX_ACTIVE_LIMIT)
            {
                audio_out_enable();
                log_debug(&serial_log, "Enabling Audio Output.\n");
            }
        }
        while (notifications--)
        {
            // Update the buffer size.
            audio_out_buffer_size -= AUDIO_TX_TRANSFER_SIZE;

            // Underflow. Disable the dac and wait for usb to catch up
            // This also happens when stopping audio playback and the buffer runs out.
            if (audio_out_buffer_size <= AUDIO_TX_UNDERFLOW_LIMIT)
            {
                audio_out_disable();
                log_warn(&serial_log, "Audio Out Underflow, Disabling Output!\n");
            }

            // Clear the overflow condition.
            if (audio_out_status & AUDIO_OUT_STS_OVERFLOW)
            {
                if (audio_out_buffer_size <= AUDIO_TX_ACTIVE_LIMIT)
                {
                    audio_out_status &= ~AUDIO_OUT_STS_OVERFLOW;
                    log_info(&serial_log, "Clearing Overflow\n");
                }
            }

            // Set the overflow condition.
            else if (audio_out_buffer_size >= AUDIO_TX_OVERFLOW_LIMIT)
            {
                audio_out_status |= AUDIO_OUT_STS_OVERFLOW;
                log_warn(&serial_log, "Audio Out Overflow!\n");
            }
        }
    }
}
void AudioTxLogging(void *pvParameters)
{
    (void)pvParameters;
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    for (ever)
    {
        vTaskDelay(xDelay);
        int buf_percent = (100 * audio_tx_size() / AUDIO_TX_BUF_SIZE);
        log_debug(&serial_log, "Buf%: %d, sts: %d    \n", buf_percent, audio_out_status);
    }
}

/* Enabling I2S will start triggering the isr for AudioTxMonitor */
void audio_out_enable(void)
{
    audio_out_status |= AUDIO_OUT_STS_ACTIVE;
    I2S_EnableTx();
    MuteControl_Write(1);
}

/* Disabling I2S will stop triggering the isr for audiotxmonitor */
void audio_out_disable(void)
{
    audio_out_status &= ~AUDIO_OUT_STS_ACTIVE;
    I2S_DisableTx();
    MuteControl_Write(0);
}

/* This isr is when each I2S dma transfer completes. This will happen
 * in regular increments of AUDIO_OUT_TRANSFER_SIZE.
 */
CY_ISR(i2s_tx_done_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xAudioTxMonitor, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void bs_dma_config(void)
{
    // Send byte swapped data to the tx buffer.
    uint8_t bs_dma_ch = DMA_BS_TX_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    uint8_t bs_dma_td[MAX_BS_TDS];
    
    uint8_t td_config = TD_INC_DST_ADR;
    const uint8_t *dst = tx_buffer;

    // Use the max transfer size for byte swap transfers.
    int remaining = AUDIO_TX_BUF_SIZE;
    uint8_t *td = bs_dma_td;
    int required_tds = (remaining + DMA_MAX_TRANSFER_SIZE - 1) / DMA_MAX_TRANSFER_SIZE;
    for (int i = 0; i < required_tds; i++)
    {
        bs_dma_td[i] = CyDmaTdAllocate();
    }

    // Infinite loop of chained tds. Use up to the max allowed td size.
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

    CyDmaChSetInitialTd(bs_dma_ch, bs_dma_td[0]);
    CyDmaChEnable(bs_dma_ch, 1u);
}

// Chain n_tds number of transactions together in an infinite loop.
static void i2s_dma_config(void)
{
    // transfer from transmit buffer to i2s fifo
    uint8_t i2s_dma_ch = DMA_I2S_TX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    uint8_t i2s_dma_td[AUDIO_TX_N_TDS];
    for (int i = 0; i < AUDIO_TX_N_TDS; i++)
    {
        i2s_dma_td[i] = CyDmaTdAllocate();
    }

    // Increment the source buffer and generate an increment every transfer.
    uint8_t td_config = TD_INC_SRC_ADR | DMA_I2S_TX__TD_TERMOUT_EN;
    const uint8_t *src = tx_buffer;

    // Chain tds in infinite loop. We get an interrupt every config->transfer_size bytes.
    for (int i = 0; i < AUDIO_TX_N_TDS; i++)
    {
        CyDmaTdSetConfiguration(i2s_dma_td[i], AUDIO_TX_TRANSFER_SIZE, i2s_dma_td[(i + 1) % AUDIO_TX_N_TDS], td_config);
        CyDmaTdSetAddress(i2s_dma_td[i], LO16((uint32_t)src), LO16((uint32_t)I2S_TX_CH0_F0_PTR));
        src += AUDIO_TX_TRANSFER_SIZE;
    }

    CyDmaChSetInitialTd(i2s_dma_ch, i2s_dma_td[0]);
    CyDmaChEnable(i2s_dma_ch, 1u);
}

// Send data to the byte swap component in a single transfer.
static void configure_source_dma(const uint8_t *src, int amount)
{
    CyDmaTdSetConfiguration(source_dma_td[0], amount, CY_DMA_DISABLE_TD, TD_INC_SRC_ADR);
    CyDmaTdSetAddress(source_dma_td[0], LO16((uint32_t)src), LO16((uint32_t)byte_swap_tx_fifo_in_ptr));
    CyDmaChSetInitialTd(source_dma_ch, source_dma_td[0]);
    CyDmaChEnable(source_dma_ch, 1u);
}
