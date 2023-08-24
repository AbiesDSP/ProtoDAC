#pragma once
#include "cptr.h"
#include <cytypes.h>

#define SERIAL_RX_MAX_TRANSFER_SIZE 4095
#define SERIAL_RX_MAX_TDS 4

#define SERIAL_RX_OVERRUN 0x2

typedef struct SerialRxConfig
{
    uint8_t *buffer;
    int size;

    uint8_t dma_ch;
    int max_dma_transfer_size;
    const reg8 *uart_rxdata_ptr;
} SerialRxConfig;

typedef struct SerialRx
{
    int capacity;
    volatile uint8_t status;

    SerialRxConfig _config;
    volatile int _buffer_size;
    Cptr _read_ptr;
    uint8_t _dma_tds[SERIAL_RX_MAX_TDS];

} SerialRx;

void serial_rx_init(SerialRx *self, const SerialRxConfig *config);
int serial_rx_receive(SerialRx *self, void *dst, int amount);

int serial_rx_buffer_size(const SerialRx *self);
int serial_rx_free_size(const SerialRx *self);
void serial_rx_isr(SerialRx *self, uint8_t transfer_size);
