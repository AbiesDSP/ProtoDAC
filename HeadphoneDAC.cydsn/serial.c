#include "serial.h"

// #define MAX_TRANSFER_SIZE 4095

// void serial_init(serial *self, uint8_t *rx_buffer, size_t rx_buffer_size, uint8_t *tx_buffer, size_t tx_buffer_size)
// {
//     cptr_init(&self->rx_buffer, rx_buffer, rx_buffer_size);
//     cptr_init(&self->tx_write_ptr, tx_buffer, tx_buffer_size);
//     cptr_init(&self->tx_read_ptr, tx_buffer, tx_buffer_size);

//     self->rx_capacity = rx_buffer_size;
//     self->tx_capacity = tx_buffer_size;

//     self->tx_buffer_size = 0;
//     self->rx_buffer_size = 0;

//     // Configure dma channels
//     self->tx_dma_ch = DMATxUART_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
//     self->rx_dma_ch = DMARxUART_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));

//     // Allocate tds.
//     int n_tds = SERIAL_MAX_TDS;
//     for (int i = 0; i < n_tds; i++)
//     {
//         self->tx_dma_tds[i] = CyDmaTdAllocate();
//         self->rx_dma_tds[i] = CyDmaTdAllocate();
//     }

//     UART_Start();
// }

// void configure_tx_dma(serial *self)
// {
//     // Send all the data in the buffer.
//     int remaining = self->tx_buffer_size;
//     self->tx_last_transfer_size = remaining;

//     uint8_t config = CY_DMA_TD_INC_SRC_ADR;

//     uint8_t *td = self->tx_dma_tds;

//     while (remaining > 0)
//     {
//         // Maximum allowed dma transfer size
//         int transfer_size = remaining > MAX_TRANSFER_SIZE ? MAX_TRANSFER_SIZE : remaining;
//         // Memory boundary of circular buffer.
//         int buf_remain = self->tx_read_ptr.end - self->tx_read_ptr.it;
//         transfer_size = transfer_size > buf_remain ? buf_remain : transfer_size;

//         remaining -= transfer_size;

//         uint8_t next_td = *(td + 1);
//         // last chained transfer.
//         if (remaining == 0)
//         {
//             config |= DMATxUART__TD_TERMOUT_EN;
//             next_td = CY_DMA_DISABLE_TD;
//         }

//         CyDmaTdSetConfiguration(*td, transfer_size, next_td, config);
//         CyDmaTdSetAddress(*td, LO16((uint32_t)self->tx_read_ptr.it), LO16((uint32_t)UART_TXDATA_PTR));

//         cptr_inc(&self->tx_read_ptr, transfer_size);
//         td++;
//     }

//     self->tx_status |= SERIAL_TX_ACTIVE;
//     CyDmaChSetInitialTd(self->tx_dma_ch, self->tx_dma_tds[0]);
//     CyDmaChEnable(self->tx_dma_ch, 1);
// }

// int serial_send(serial *self, const void *src, int amount)
// {
//     // Check for free space?
//     // Copy data into the buffer
//     cptr_copy_into(&self->tx_write_ptr, src, amount);
//     self->tx_buffer_size += amount;

//     //
//     if ((self->tx_status & SERIAL_TX_ACTIVE) == 0)
//     {
//         configure_tx_dma(self);
//     }
//     return amount;
// }

// int serial_receive(serial *self, void *dst, int amount)
// {
//     return amount;
// }

// void serial_service_tx_isr(serial *self)
// {
//     self->tx_buffer_size -= self->tx_last_transfer_size;
//     self->tx_last_transfer_size = 0;

//     self->tx_status &= ~SERIAL_TX_ACTIVE;

//     // Start another transaction if there are bytes waiting to send.
//     if (self->tx_buffer_size > 0)
//     {
//         configure_tx_dma(self);
//     }
// }

// void serial_service_rx_isr(serial *self, int transfer_size)
// {
// }
