#pragma once
#include "cptr.h"
#include <cytypes.h>

#define SERIAL_MAX_TDS 4

typedef struct serial serial;
struct serial
{
    cptr rx_buffer;
    int rx_capacity;
    int rx_buffer_size;

    cptr tx_write_ptr;
    cptr tx_read_ptr;
    int tx_capacity;
    // Number of bytes in the tx buffer waiting to be sent.
    int tx_buffer_size;

    int tx_last_transfer_size;
    int tx_status;

    // DMA Config
    uint8_t tx_dma_ch;
    uint8_t tx_dma_tds[SERIAL_MAX_TDS];

    uint8_t rx_dma_ch;
    uint8_t rx_dma_tds[SERIAL_MAX_TDS];
};

#define SERIAL_TX_IDLE 0x0
#define SERIAL_TX_ACTIVE 0x1

void serial_init(serial *self, uint8_t *rx_buffer, size_t rx_buffer_size, uint8_t *tx_buffer, size_t tx_buffer_size);

static inline int serial_tx_free_size(const serial *self)
{
    return self->tx_capacity - self->tx_buffer_size;
}

static inline int serial_rx_free_size(const serial *self)
{
    return self->rx_capacity - self->rx_buffer_size;
}

int serial_send(serial *self, const void *src, int amount);
int serial_receive(serial *self, void *dst, int amount);

void serial_service_tx_isr(serial *self);
void serial_service_rx_isr(serial *self, int transfer_size);

//
// CY_ISR_PROTO(serial_rx_isr);
// CY_ISR_PROTO(serial_tx_isr);
