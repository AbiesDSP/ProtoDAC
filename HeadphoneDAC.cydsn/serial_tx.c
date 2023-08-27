#include "serial_tx.h"

#include <UART.h>
#include <DMATxUART_dma.h>
#include "uart_tx_isr.h"

static uint8_t transmit_buffer[SERIAL_TX_BUFFER_SIZE];

static uint8_t dma_ch;
static uint8_t dma_tds[SERIAL_TX_MAX_TDS];

static TaskHandle_t xSerialSender = NULL;
SemaphoreHandle_t serial_tx_lock;

#define TX_DONE_BIT 0x1
#define NEW_DATA_BIT 0x2

static void configure_dma(void);
CY_ISR_PROTO(transfer_done_isr);

void serial_tx_init(void)
{
    //
    dma_ch = DMATxUART_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    // Allocate tds for dma.
    for (int i = 0; i < SERIAL_TX_MAX_TDS; i++)
    {
        dma_tds[i] = CyDmaTdAllocate();
    }

    UART_Start();
    uart_tx_isr_StartEx(transfer_done_isr);

    serial_tx_lock = xSemaphoreCreateMutex();
}

static int buffer_size = 0;
static Cptr write_ptr;
static Cptr read_ptr;

int serial_send(const void *src, int amount)
{
    // Check for free space?
    // Copy data into the buffer
//    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(1);
    
//    if (xSemaphoreTake(serial_tx_lock, xMaxBlockTime))
//    {
        cptr_copy_into(&write_ptr, src, amount);
        buffer_size += amount;

        // xTaskNotify(xSerialSender, NEW_DATA_BIT, eSetBits);
        xTaskNotifyGive(xSerialSender);
        // xSemaphoreGive(serial_tx_lock);
//    }
//    else
//    {
//        amount = 0;
//    }
    

    return amount;
}

void SerialSender(void *pvParameters)
{
    (void)pvParameters;

    xSerialSender = xTaskGetCurrentTaskHandle();

    // Initialize transmit buffer circular pointers.
    cptr_init(&write_ptr, transmit_buffer, SERIAL_TX_BUFFER_SIZE);
    cptr_init(&read_ptr, transmit_buffer, SERIAL_TX_BUFFER_SIZE);

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(10);

    uint32_t notifications = 0;

    int last_transfer_size = 0;

    for (ever)
    {
        notifications = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        //
        if(notifications)
        {
            // Tx transfer finished.
            buffer_size -= last_transfer_size;
            last_transfer_size = 0;
            //
            if (buffer_size)
            {
                last_transfer_size = buffer_size;
                configure_dma();
            }
        }
        // Timed out and we have data waiting.
        else if (buffer_size)
        {
            last_transfer_size = buffer_size;
            configure_dma();
        }
    }
}

static void configure_dma(void)
{
    // Send all the data in the buffer.
    int remaining = buffer_size;

    uint8_t config = TD_INC_SRC_ADR;

    uint8_t *td = dma_tds;

    while (remaining > 0)
    {
        // Maximum allowed dma transfer size
        int transfer_size = remaining > SERIAL_TX_MAX_TRANSFER_SIZE ? SERIAL_TX_MAX_TRANSFER_SIZE : remaining;
        // Memory boundary of circular buffer.
        int buf_remain = read_ptr.end - read_ptr.ptr;
        transfer_size = transfer_size > buf_remain ? buf_remain : transfer_size;

        remaining -= transfer_size;

        uint8_t next_td = *(td + 1);
        // last chained transfer.
        if (remaining == 0)
        {
            config |= DMATxUART__TD_TERMOUT_EN;
            next_td = CY_DMA_DISABLE_TD;
        }

        CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)read_ptr.ptr), LO16((uint32_t)UART_TXDATA_PTR));

        cptr_inc(&read_ptr, transfer_size);
        td++;
    }

    CyDmaChSetInitialTd(dma_ch, dma_tds[0]);
    CyDmaChEnable(dma_ch, 1);
}

int serial_tx_buffer_size(void)
{
    return buffer_size;
}

int serial_tx_free_size(void)
{
    return SERIAL_TX_BUFFER_SIZE - buffer_size;
}

CY_ISR(transfer_done_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xSerialSender, &xHigherPriorityTaskWoken);
    // xTaskNotifyFromISR(xSerialSender, TX_DONE_BIT, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
