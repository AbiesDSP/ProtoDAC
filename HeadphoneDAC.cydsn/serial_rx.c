#include "serial_rx.h"
#include "project_config.h"

#include <UART.h>
#include <DMARxUART_dma.h>

static uint8_t rx_buffer[SERIAL_RX_BUFFER_SIZE];

static int rx_buffer_size = 0;
static uint8_t rx_status = 0;

// static Cptr write_ptr;
static Cptr read_ptr;

static TaskHandle_t xSerialReceiver = NULL;

CY_ISR_PROTO(flush_isr);

void SerialReceiver(void *pvParameters)
{
    (void)pvParameters;

    xSerialReceiver = xTaskGetCurrentTaskHandle();

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(10);

    for (ever)
    {
        if (ulTaskNotifyTake(pdTRUE, xMaxBlockTime))
        {
            rx_buffer_size += Flush_TransferSize();
        }
    }
}

// Write data into a looping circular buffer.
static void configure_dma(void);

void serial_rx_init(void)
{
    rx_buffer_size = 0;
    rx_status = 0;

    cptr_init(&read_ptr, rx_buffer, SERIAL_RX_BUFFER_SIZE);

    // Start dma.
    configure_dma();
    flush_isr_StartEx(flush_isr);

    UART_Start();
}

int serial_receive(void *dst, int amount)
{
    cptr_copy_from(&read_ptr, dst, amount);
    rx_buffer_size -= amount;
    return amount;
}

int serial_rx_buffer_size(void)
{
    return rx_buffer_size;
}

int serial_rx_free_size(void)
{
    return SERIAL_RX_BUFFER_SIZE - rx_buffer_size;
}

CY_ISR(flush_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xSerialReceiver, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    // rx_buffer_size += transfer_size;
}

static void configure_dma(void)
{
    uint8_t dma_ch = DMARxUART_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    uint8_t dma_tds[SERIAL_RX_MAX_TDS];
    // Allocate tds for dma.
    for (int i = 0; i < SERIAL_RX_MAX_TDS; i++)
    {
        dma_tds[i] = CyDmaTdAllocate();
    }

    int remaining = SERIAL_RX_BUFFER_SIZE;

    uint8_t config = TD_INC_DST_ADR;
    uint8_t *td = dma_tds;

    while (remaining > 0)
    {
        // Maximum allowed dma transfer size
        int transfer_size = remaining > DMA_MAX_TRANSFER_SIZE ? DMA_MAX_TRANSFER_SIZE : remaining;
        remaining -= transfer_size;

        uint8_t next_td = remaining == 0 ? dma_tds[0] : *(td + 1);

        CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)UART_RXDATA_PTR), LO16((uint32_t)read_ptr.ptr));

        cptr_inc(&read_ptr, transfer_size);
        td++;
    }

    CyDmaChSetInitialTd(dma_ch, dma_tds[0]);
    CyDmaChEnable(dma_ch, 1);
}
