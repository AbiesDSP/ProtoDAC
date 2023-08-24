#include "serial_tx.h"
#include "CyDmac.h"

void serial_tx_init(SerialTx *self, const SerialTxConfig *config)
{
    self->_config = *config;

    self->_buffer_size = 0;
    self->capacity = self->_config.size;
    self->status = 0;

    // Initialize transmit buffer circular pointers.
    cptr_init(&self->_write_ptr, self->_config.buffer, self->capacity);
    cptr_init((Cptr*)&self->_read_ptr, self->_config.buffer, self->capacity);

    // Allocate tds for dma.
    for (int i = 0; i < SERIAL_TX_MAX_TDS; i++)
    {
        self->_dma_tds[i] = CyDmaTdAllocate();
    }
}

static void configure_dma(SerialTx *self);

int serial_tx_send(SerialTx *self, const void *src, int amount)
{
    // Check for free space?
    // Copy data into the buffer
    cptr_copy_into(&self->_write_ptr, src, amount);
    self->_buffer_size += amount;

    // If the dma isn't currently transferring data, start a new transaction.
    // Otherwise, a dma transfer will automatically be started in the isr after the current transfer finishes.
    if ((self->status & SERIAL_TX_ACTIVE) == 0)
    {
        configure_dma(self);
    }

    return amount;
}

static void configure_dma(SerialTx *self)
{
    // Send all the data in the buffer.
    int remaining = self->_buffer_size;
    self->_last_transfer_size = remaining;

    uint8_t config = TD_INC_SRC_ADR;

    uint8_t *td = self->_dma_tds;

    while (remaining > 0)
    {
        // Maximum allowed dma transfer size
        int transfer_size = remaining > self->_config.max_dma_transfer_size ? self->_config.max_dma_transfer_size : remaining;
        // Memory boundary of circular buffer.
        int buf_remain = self->_read_ptr.end - self->_read_ptr.ptr;
        transfer_size = transfer_size > buf_remain ? buf_remain : transfer_size;

        remaining -= transfer_size;

        uint8_t next_td = *(td + 1);
        // last chained transfer.
        if (remaining == 0)
        {
            config |= self->_config.td_termout_en;
            next_td = CY_DMA_DISABLE_TD;
        }

        CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
        CyDmaTdSetAddress(*td, LO16((uint32_t)self->_read_ptr.ptr), LO16((uint32_t)self->_config.uart_txdata_ptr));

        cptr_inc((Cptr*)&self->_read_ptr, transfer_size);
        td++;
    }

    self->status |= SERIAL_TX_ACTIVE;
    CyDmaChSetInitialTd(self->_config.dma_ch, self->_dma_tds[0]);
    CyDmaChEnable(self->_config.dma_ch, 1);
}

int serial_tx_buffer_size(const SerialTx *self)
{
    return self->_buffer_size;
}

int serial_tx_free_size(const SerialTx *self)
{
    return self->capacity - self->_buffer_size;
}

void serial_tx_isr(SerialTx *self)
{
    self->_buffer_size -= self->_last_transfer_size;
    self->_last_transfer_size = 0;

    self->status &= ~SERIAL_TX_ACTIVE;

    // Start another transaction if there are bytes waiting to send.
    if (self->_buffer_size > 0)
    {
        configure_dma(self);
    }
}
