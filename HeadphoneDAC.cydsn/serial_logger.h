#pragma once
#include "serial_tx.h"
#include "logging.h"

typedef struct SerialLogHandler
{
    LogHandler handler;
    SerialTx *serial_tx;
} SerialLogHandler;

void serial_log_handler_init(SerialLogHandler *self, SerialTx *serial_tx);
