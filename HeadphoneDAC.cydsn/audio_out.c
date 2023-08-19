#include "audio_out.h"
#include "usb.h"
#include "CyDmac.h"
#include "byte_swap_tx.h"
#include "byte_swap_tx_defs.h"
#include "USBFS.h"
#include "I2S.h"
#include "DMA_USB_TX_dma.h"
#include "DMA_BS_TX_dma.h"
#include "DMA_I2S_TX_dma.h"
#include "i2s_tx_isr.h"
#include "mute.h"

#define MAX_TRANSFER_SIZE 4095
#define FIFO_HALF_FULL_MASK 0x0C
#define N_BS_TD ((AUDIO_OUT_BUF_SIZE + (MAX_TRANSFER_SIZE - 1)) / MAX_TRANSFER_SIZE)

// uint8_t audio_out_process[AUDIO_OUT_PROCESS_SIZE];
uint16_t audio_out_count = 0;

uint8_t audio_out_buf[AUDIO_OUT_BUF_SIZE];
volatile uint16_t audio_out_buffer_size;
volatile uint8_t audio_out_status;
volatile uint8_t audio_out_active;

volatile int audio_out_update_flag = 0;

static uint8_t usb_tx_dma_td[1];
static uint8_t bs_tx_dma_td[N_BS_TD];
static uint8_t i2s_tx_dma_td[AUDIO_OUT_N_TD];

static uint8_t usb_tx_dma_ch;
static uint8_t bs_tx_dma_ch;
static uint8_t i2s_tx_dma_ch;

static void bs_dma_config(void);
static void i2s_dma_config(void);

void audio_out_init(void)
{
    uint16_t i;

    usb_tx_dma_ch = DMA_USB_TX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    bs_tx_dma_ch = DMA_BS_TX_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    i2s_tx_dma_ch = DMA_I2S_TX_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));

    // Allocate all TDs
    usb_tx_dma_td[0] = CyDmaTdAllocate();
    for (i = 0; i < N_BS_TD; i++)
    {
        bs_tx_dma_td[i] = CyDmaTdAllocate();
    }
    for (i = 0; i < AUDIO_OUT_N_TD; i++)
    {
        i2s_tx_dma_td[i] = CyDmaTdAllocate();
    }

    // Initiliaze the buffer.
    for (i = 0; i < AUDIO_OUT_BUF_SIZE; i++)
    {
        audio_out_buf[i] = 0;
    }

    // Initialize buffer management.
    audio_out_count = 0;
    audio_out_buffer_size = 0;
    audio_out_status = 0;
    audio_out_active = 0;

    byte_swap_tx_DP_F0_SET_LEVEL_MID;
    byte_swap_tx_DP_F1_SET_LEVEL_MID;
    I2S_TX_AUX_CONTROL_REG = I2S_TX_AUX_CONTROL_REG | FIFO_HALF_FULL_MASK;
}

void audio_out_start(void)
{
    audio_out_init();

    bs_dma_config();
    i2s_dma_config();

    i2s_tx_isr_StartEx(i2s_tx_done_isr);

    // BS channel is always on. Other channels are intermittent.
    CyDmaChEnable(bs_tx_dma_ch, 1u);
    I2S_Start();
}

void audio_out_update(void)
{
    uint8_t int_status = 0;
    uint16_t buf_size = 0;

    // We've received bytes from USB. We need to process the data, then retransmit it.
    // Copy to the processing buf, then set the update flag.
    audio_out_count = USBFS_GetEPCount(AUDIO_OUT_EP);

    // Process data from usb here.

    if (audio_out_status & AUDIO_OUT_STS_RESET)
    {
        audio_out_status &= ~AUDIO_OUT_STS_RESET;
        audio_out_buffer_size = 0;
    }

    // Start a dma transaction
    CyDmaTdSetConfiguration(usb_tx_dma_td[0], audio_out_count, CY_DMA_DISABLE_TD, (TD_INC_SRC_ADR | DMA_USB_TX__TD_TERMOUT_EN));
    CyDmaTdSetAddress(usb_tx_dma_td[0], LO16((uint32_t)usb_audio_out_buf), LO16((uint32_t)byte_swap_tx_fifo_in_ptr));
    CyDmaChSetInitialTd(usb_tx_dma_ch, usb_tx_dma_td[0]);
    CyDmaChEnable(usb_tx_dma_ch, 1u);

    int_status = CyEnterCriticalSection();
    audio_out_buffer_size += audio_out_count;
    buf_size = audio_out_buffer_size;
    CyExitCriticalSection(int_status);

    // If it's off and we're over half full, start it up.
    if ((audio_out_status & AUDIO_OUT_STS_ACTIVE) == 0u && buf_size >= AUDIO_OUT_ACTIVE_LIMIT)
    {
        audio_out_enable();
    }
    audio_out_update_flag = 1;
    //    audio_out_transmit();
}

void audio_out_transmit(void)
{
}

void audio_out_enable(void)
{
    // Only enable if we're currently enabled
    if ((audio_out_status & AUDIO_OUT_STS_ACTIVE) == 0u)
    {
        audio_out_status |= AUDIO_OUT_STS_ACTIVE;
        audio_out_active = 1;
        CyDmaChEnable(i2s_tx_dma_ch, 1u);
        I2S_EnableTx();
        mute_Write(1);
    }
}

void audio_out_disable(void)
{
    // Only disable if active
    if (audio_out_status & AUDIO_OUT_STS_ACTIVE)
    {
        I2S_DisableTx();
        // CyDelayUs(20);
        CyDmaChDisable(i2s_tx_dma_ch);
        audio_out_status &= ~AUDIO_OUT_STS_ACTIVE;
        audio_out_active = 0;
        audio_out_buffer_size = 0;
        mute_Write(0);
    }
}

static void bs_dma_config(void)
{
    uint16_t i = 0;
    uint16_t remain, transfer_size;

    remain = AUDIO_OUT_BUF_SIZE;
    while (remain > 0)
    {
        transfer_size = remain > MAX_TRANSFER_SIZE ? MAX_TRANSFER_SIZE : remain;
        CyDmaTdSetConfiguration(bs_tx_dma_td[i], transfer_size, bs_tx_dma_td[(i + 1) % N_BS_TD], (TD_INC_DST_ADR | DMA_BS_TX__TD_TERMOUT_EN));
        CyDmaTdSetAddress(bs_tx_dma_td[i], LO16((uint32_t)byte_swap_tx_fifo_out_ptr), LO16((uint32_t)&audio_out_buf[i * MAX_TRANSFER_SIZE]));
        i++;
        remain -= transfer_size;
    }
    CyDmaChSetInitialTd(bs_tx_dma_ch, bs_tx_dma_td[0]);
}

static void i2s_dma_config(void)
{
    uint16_t i = 0;
    // Set up chained tds to loop around entire audio out buffer.
    for (i = 0; i < AUDIO_OUT_N_TD; i++)
    {
        CyDmaTdSetConfiguration(i2s_tx_dma_td[i], AUDIO_OUT_TRANSFER_SIZE, i2s_tx_dma_td[(i + 1) % AUDIO_OUT_N_TD], (TD_INC_SRC_ADR | DMA_I2S_TX__TD_TERMOUT_EN));
        CyDmaTdSetAddress(i2s_tx_dma_td[i], LO16((uint32_t)&audio_out_buf[i * AUDIO_OUT_TRANSFER_SIZE]), LO16((uint32_t)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_tx_dma_ch, i2s_tx_dma_td[0]);
}

// Byte swap transfer completed. Not quite working...
CY_ISR_PROTO(bs_tx_done_isr)
{
}

/* This isr is when each I2S transfer completes. This will happen
 * in regular increments of AUDIO_OUT_TRANSFER_SIZE. So we can
 * decrease the buffer size by that much.
 */
CY_ISR(i2s_tx_done_isr)
{
    audio_out_buffer_size -= AUDIO_OUT_TRANSFER_SIZE;
    // About to underflow. Oh nooooo.
    if (audio_out_buffer_size <= AUDIO_OUT_LOW_LIMIT)
    {
        // What do?
        audio_out_status |= AUDIO_OUT_STS_UNDERFLOW;
        audio_out_status |= AUDIO_OUT_STS_RESET;
        audio_out_disable();
    }
    else if (audio_out_buffer_size >= AUDIO_OUT_HIGH_LIMIT)
    {
        audio_out_status |= AUDIO_OUT_STS_OVERFLOW;
        audio_out_status |= AUDIO_OUT_STS_RESET;
        audio_out_disable();
    }
}
