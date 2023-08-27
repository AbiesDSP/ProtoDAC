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

#define FIFO_HALF_FULL_MASK 0x0C

static int tx_buffer_size = 0;
static uint8_t tx_status = 0;

// Transfers audio data to the byte swap component
static uint8_t source_dma_ch;
static uint8_t source_dma_td[1];

// Optional byte-swap operation to swap the endianness of 24 bit data.
// Transfers data from memory to the byte-swap component,
// Then the endian-swapped bytes are transferred to the transmit buffer.

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
static QueueHandle_t xTxBufferDeltaQueue = NULL;

int audio_tx_size(void)
{
    return tx_buffer_size;
}
uint8_t audio_tx_status(void)
{
    return tx_status;
}

// Configure and start up all components for audio
void audio_tx_init(void)
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
    tx_buffer_size = 0;
    tx_status = 0;

    xTxBufferDeltaQueue = xQueueCreate(AUDIO_TX_DELTA_QUEUE_SIZE, sizeof(int));

    // Set UDB fifos to use half full flags.
    byte_swap_tx_DP_F0_SET_LEVEL_MID;
    byte_swap_tx_DP_F1_SET_LEVEL_MID;
    I2S_TX_AUX_CONTROL_REG |= FIFO_HALF_FULL_MASK;
}

// Send audio data to the byte swap component, which then gets sent to the transmit buffer.
void AudioTx(void *source_buffer)
{
    uint32_t source_buffer_size = 0;
    const TickType_t xMaxWait = pdMS_TO_TICKS(AUDIO_TX_TX_MAX_WAIT);

    for (ever)
    {
        // Wait for a notification that more data is available from the source.
        if (xTaskNotifyWait(0, UINT32_MAX, &source_buffer_size, xMaxWait))
        {
            // If we're in an overflow state and above half full, drop any incoming data.
            if (tx_status & AUDIO_OUT_STS_OVERFLOW)
            {
                log_error(&serial_log, "Overflowed! Dropping Data!\n");
            }
            else
            {
                // Start a dma transaction to send data to the byte swap component
                configure_source_dma(source_buffer, source_buffer_size);

                // Inform the AudioTxMonitor task that bytes were added to the transmit buffer.
                xQueueSend(xTxBufferDeltaQueue, &source_buffer_size, 0);
            }
        }
    }
}

// Monitor the tx buffer size to determine when to enable or disable audio output.
// Handle overflow/underflow gracefully
// Modify the buffer size in response to new data being added and the i2s isr.
void AudioTxMonitor(void *pvParameters)
{
    (void)pvParameters;

    // Set based on the transfer size.
    const TickType_t xMaxWait = pdMS_TO_TICKS(AUDIO_TX_MONITOR_MAX_WAIT);
    int buffer_delta_update = 0;

    for (ever)
    {
        if (xQueueReceive(xTxBufferDeltaQueue, &buffer_delta_update, xMaxWait))
        {
            tx_buffer_size += buffer_delta_update;

            // Underflow. Disable the dac and wait for usb to catch up
            // This also happens when stopping audio playback and the buffer runs out.
            if ((tx_status & AUDIO_OUT_STS_ACTIVE) && tx_buffer_size <= AUDIO_TX_UNDERFLOW_LIMIT)
            {
                audio_tx_disable();
                log_warn(&serial_log, "Audio Out Underflow, Disabling Output!\n");
            }

            // Clear the overflow condition.
            if (tx_status & AUDIO_OUT_STS_OVERFLOW)
            {
                if (tx_buffer_size <= AUDIO_TX_ACTIVE_LIMIT)
                {
                    tx_status &= ~AUDIO_OUT_STS_OVERFLOW;
                    log_info(&serial_log, "Clearing Overflow\n");
                }
            }

            // Set the overflow condition.
            else if (tx_buffer_size >= AUDIO_TX_OVERFLOW_LIMIT)
            {
                tx_status |= AUDIO_OUT_STS_OVERFLOW;
                log_warn(&serial_log, "Audio Out Overflow!\n");
            }
        }
        else
        {
            // Timed out. I2S is not currently enabled and no usb packets coming in.
            if (!(tx_status & AUDIO_OUT_STS_ACTIVE) && tx_buffer_size >= AUDIO_TX_ACTIVE_LIMIT)
            {
                audio_tx_enable();
                log_info(&serial_log, "Enabling Audio Output.\n");
            }
        }
    }
}
void AudioTxLogging(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t xDelay = pdMS_TO_TICKS(AUDIO_TX_LOGGING_INTERVAL);
    for (ever)
    {
        vTaskDelay(xDelay);
        int buf_percent = (100 * audio_tx_size() / AUDIO_TX_BUF_SIZE);
        log_debug(&serial_log, "Buf%%: %d, sts: %d    \n", buf_percent, tx_status);
    }
}

/* Enabling I2S will start triggering the isr for AudioTxMonitor */
void audio_tx_enable(void)
{
    tx_status |= AUDIO_OUT_STS_ACTIVE;
    I2S_EnableTx();
    MuteControl_Write(1);
}

/* Disabling I2S will stop triggering the isr for audiotxmonitor */
void audio_tx_disable(void)
{
    tx_status &= ~AUDIO_OUT_STS_ACTIVE;
    I2S_DisableTx();
    MuteControl_Write(0);
}

/* This isr is when each I2S dma transfer completes. This will happen
 * in regular increments of AUDIO_OUT_TRANSFER_SIZE.
 */
static const int i2s_isr_delta = -AUDIO_TX_TRANSFER_SIZE;
CY_ISR(i2s_tx_done_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Inform the xAudioTxMonitor task that a transfer completed.
    xQueueSendFromISR(xTxBufferDeltaQueue, &i2s_isr_delta, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void bs_dma_config(void)
{
    // Send byte swapped data to the tx buffer.
    // Init dma. Transferring 1 byte at a time from byte swap to sram.
    uint8_t bs_dma_ch = DMA_BS_TX_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    uint8_t bs_dma_td[AUDIO_TX_MAX_BS_TDS];

    // Use the max transfer size for byte swap transfers. Allocate enough for the whole buffer.
    for (int i = 0; i < AUDIO_TX_MAX_BS_TDS; i++)
    {
        bs_dma_td[i] = CyDmaTdAllocate();
    }

    // Use whole buffer.
    int remaining = AUDIO_TX_BUF_SIZE;
    // Increment the sram address when copying.
    uint8_t td_config = TD_INC_DST_ADR;
    // starting td
    uint8_t *td = bs_dma_td;
    // transfer dst.
    const uint8_t *dst = tx_buffer;

    // Infinite loop of chained tds. Use up to the max allowed td size.
    while (remaining > 0)
    {
        int transfer_size = remaining > DMA_MAX_TRANSFER_SIZE ? DMA_MAX_TRANSFER_SIZE : remaining;
        remaining -= transfer_size;
        // Loop the last td to the first.
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
