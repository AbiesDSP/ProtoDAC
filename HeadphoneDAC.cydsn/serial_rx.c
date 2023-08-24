#include "serial_rx.h"

#include <UART.h>
#include <DMARxUART_dma.h>

// Write data into a looping circular buffer.
static void configure_dma(SerialRx *self)
{
    int remaining = self->capacity;

    uint8_t config = TD_INC_DST_ADR;
    uint8_t *td = self->_dma_tds;

    while (remaining > 0)
    {
        // Maximum allowed dma transfer size
        int transfer_size = remaining > self->_config.max_dma_transfer_size ? self->_config.max_dma_transfer_size : remaining;
        remaining -= transfer_size;

        uint8_t next_td = *(td + 1);
        // last chained transfer.
        if (remaining == 0)
        {
            next_td = self->_dma_tds[0];
        }

        CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)self->_config.uart_rxdata_ptr), LO16((uint32_t)self->_read_ptr.ptr));

        cptr_inc(&self->_read_ptr, transfer_size);
        td++;
    }

    CyDmaChSetInitialTd(self->_config.dma_ch, self->_dma_tds[0]);
    CyDmaChEnable(self->_config.dma_ch, 1);
}

void serial_rx_init(SerialRx *self, const SerialRxConfig *config)
{
    self->_config = *config;

    self->_buffer_size = 0;
    self->capacity = self->_config.size;
    self->status = 0;

    cptr_init(&self->_read_ptr, self->_config.buffer, self->capacity);

    // Allocate tds for dma.
    for (int i = 0; i < SERIAL_RX_MAX_TDS; i++)
    {
        self->_dma_tds[i] = CyDmaTdAllocate();
    }

    // Start dma.
    configure_dma(self);
}

int serial_rx_receive(SerialRx *self, void *dst, int amount)
{
    cptr_copy_from(&self->_read_ptr, dst, amount);
    self->_buffer_size -= amount;
    return amount;
}

int serial_rx_buffer_size(const SerialRx *self)
{
    return self->_buffer_size;
}

int serial_rx_free_size(const SerialRx *self)
{
    return self->capacity - self->_buffer_size;
}

void serial_rx_isr(SerialRx *self, uint8_t transfer_size)
{
    self->_buffer_size += transfer_size;
}
