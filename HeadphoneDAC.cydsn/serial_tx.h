#pragma once
#include "cptr.h"
#include <cytypes.h>

#define SERIAL_TX_ACTIVE 0x1
#define SERIAL_TX_OVERRUN 0x2

#define SERIAL_TX_MAX_TDS 4

typedef struct SerialTxConfig
{
    uint8_t *buffer;
    int size;

    uint8_t dma_ch;
    int max_dma_transfer_size;
    const reg8 *uart_txdata_ptr;
    uint8_t td_termout_en;
} SerialTxConfig;

typedef struct SerialTx
{
    int capacity;
    volatile uint8_t status;

    SerialTxConfig _config;
    volatile int _buffer_size;
    Cptr _write_ptr;
    volatile Cptr _read_ptr;
    volatile int _last_transfer_size;

    uint8_t _dma_tds[SERIAL_TX_MAX_TDS];

} SerialTx;

void serial_tx_init(SerialTx *self, const SerialTxConfig *config);
int serial_tx_send(SerialTx *self, const void *src, int amount);
int serial_tx_buffer_size(const SerialTx *self);
int serial_tx_free_size(const SerialTx *self);

void serial_tx_isr(SerialTx *self);
