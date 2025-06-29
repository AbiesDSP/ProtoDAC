#include "project_config.h"

#include "audio_tx.h"
#include "audio_proc.h"
#include "cptr.h"
#include "knobs.h"
#include "loggers.h"

#include <I2S.h>
#include <DMA_I2S_TX_dma.h>
#include <DMA_Copy_dma.h>
#include <i2s_tx_isr.h>

#include <stdlib.h>

#define FIFO_HALF_FULL_MASK 0x0C

static int tx_buffer_size = 0;
static uint8_t tx_status = 0;

// I2S Transmit dma. Transfers data from the transmit buffer to the i2s component.
// dma tds transfers of equal sizes will be chained and there will be an interrupt for each transfer.
// This interrupt gives an update on the buffer size.

// Configure i2s dma
static void i2s_dma_config(void);

// ISRs for TopDesign peripherals.
CY_ISR_PROTO(i2s_tx_done_isr);

static uint8_t tx_buffer[AUDIO_TX_BUF_SIZE];
static QueueHandle_t TxBufferDeltaQueue = NULL;
static uint8_t swap_buf[USB_AUDIO_EP_BUF_SIZE];

static Cptr tx_write_ptr;

int audio_tx_size(void)
{
    return tx_buffer_size;
}
uint8_t audio_tx_status(void)
{
    return tx_status;
}

static uint8_t copy_dma_ch;
static uint8_t copy_dma_tds[2];

// Configure and start up all components for audio
void audio_tx_init(void)
{
    // i2s buffer is circular
    cptr_init(&tx_write_ptr, tx_buffer, AUDIO_TX_BUF_SIZE);
    // Copy memory to memory.
    copy_dma_ch = DMA_Copy_DmaInitialize(0, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (int i = 0; i < 2; i++)
    {
        copy_dma_tds[i] = CyDmaTdAllocate();
    }
    CyDmaChSetExtendedAddress(copy_dma_ch, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    CyDmaChSetInitialTd(copy_dma_ch, copy_dma_tds[0]);
    CyDmaChEnable(copy_dma_ch, 1);

    // Initialize the i2s modules
    i2s_dma_config();
    i2s_tx_isr_StartEx(i2s_tx_done_isr);
    I2S_Start();
    I2S_DisableTx();

    // Initialize buffer management.
    tx_buffer_size = 0;
    tx_status = 0;

    TxBufferDeltaQueue = xQueueCreate(AUDIO_TX_DELTA_QUEUE_SIZE, sizeof(int));

    // Set UDB fifos to use half full flags.
    I2S_TX_AUX_CONTROL_REG |= FIFO_HALF_FULL_MASK;
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

void copy_into_with_dma(Cptr *write_ptr, const uint8_t *src, int amount)
{
    uint8_t *td = copy_dma_tds;
    uint8_t config = TD_INC_DST_ADR | TD_INC_SRC_ADR;

    int nrq = 0;
    while (amount > 0)
    {
        // Number of bytes until the end of the memory in the circular buffer.
        int remaining = write_ptr->end - write_ptr->ptr;

        // Split in to two transfers if it would cross a memory boundary
        int transfer_size = amount > remaining ? remaining : amount;
        amount -= transfer_size;

        uint8_t next_td = amount == 0 ? copy_dma_tds[0] : *(td + 1);
        if (amount == 0)
        {
            config |= DMA_Copy__TD_TERMOUT_EN;
        }

        CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)src), LO16((uint32_t)write_ptr->ptr));

        td++;
        nrq++;
        cptr_inc(write_ptr, transfer_size);
        src += transfer_size;
    }
    // Generate 2 requests if chained, 1 otherwise.
    for (int i = 0; i < nrq; i++)
    {
        CyDmaChSetRequest(copy_dma_ch, CY_DMA_CPU_REQ);
    }
}

float lsrc[98];
float rsrc[98];

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
                log_error(&main_log, "Overflowed! Dropping Data!\n");
            }
            else
            {
                // Number of samples per channel in the packet.
                int n_samples = source_buffer_size / 6;

                // Unpack the usb data into floats.
                unpack_usb_data_float(source_buffer, n_samples, lsrc, 0);
                unpack_usb_data_float(source_buffer, n_samples, rsrc, 1);

                float gain = get_knob(0);
                // float gain = 1.0;

                volume(lsrc, lsrc, n_samples, gain);
                volume(rsrc, rsrc, n_samples, gain);

                // Process the data into the swap buffer
                pack_i2s_data_float(swap_buf, n_samples, lsrc, 0);
                pack_i2s_data_float(swap_buf, n_samples, rsrc, 1);

                // Copy the swap buffer into the i2s transmit buffer.
                copy_into_with_dma(&tx_write_ptr, swap_buf, source_buffer_size);
                // cptr_copy_into(&tx_write_ptr, swap_buf, source_buffer_size);

                // Inform the AudioTxMonitor task that bytes were added to the transmit buffer.
                xQueueSend(TxBufferDeltaQueue, &source_buffer_size, 0);
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
        // Receive updates on the buffer size changes from AudioTx or from the I2S Tx ISR.
        if (xQueueReceive(TxBufferDeltaQueue, &buffer_delta_update, xMaxWait))
        {
            tx_buffer_size += buffer_delta_update;

            // Underflow. Disable the dac and wait for usb to catch up
            // This also happens when stopping audio playback and the buffer runs out.
            if ((tx_status & AUDIO_OUT_STS_ACTIVE) && tx_buffer_size <= AUDIO_TX_UNDERFLOW_LIMIT)
            {
                audio_tx_disable();
                log_warn(&main_log, "Audio Out Underflow, Disabling Output!\n");
            }

            // Clear the overflow condition.
            if (tx_status & AUDIO_OUT_STS_OVERFLOW)
            {
                if (tx_buffer_size <= AUDIO_TX_ACTIVE_LIMIT)
                {
                    tx_status &= ~AUDIO_OUT_STS_OVERFLOW;
                    log_info(&main_log, "Clearing Overflow\n");
                }
            }

            // Set the overflow condition.
            else if (tx_buffer_size >= AUDIO_TX_OVERFLOW_LIMIT)
            {
                tx_status |= AUDIO_OUT_STS_OVERFLOW;
                log_warn(&main_log, "Audio Out Overflow!\n");
            }
        }
        else
        {
            // Timed out. I2S is not currently enabled and no usb packets coming in.
            if (!(tx_status & AUDIO_OUT_STS_ACTIVE) && tx_buffer_size >= AUDIO_TX_ACTIVE_LIMIT)
            {
                audio_tx_enable();
                log_info(&main_log, "Enabling Audio Output.\n");
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
        log_info(&main_log, "Buf%%: %d\n", buf_percent);
    }
}

/* This isr is when each I2S dma transfer completes. This will happen
 * in regular increments of AUDIO_OUT_TRANSFER_SIZE.
 */
static const int i2s_isr_delta = -AUDIO_TX_TRANSFER_SIZE;
CY_ISR(i2s_tx_done_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Inform the xAudioTxMonitor task that a transfer completed.
    xQueueSendFromISR(TxBufferDeltaQueue, &i2s_isr_delta, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
