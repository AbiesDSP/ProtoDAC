#include "serial_logger.h"

int serial_handler_write(LogHandler *_handler, const char *src, int amount)
{
    SerialLogHandler *handler = (SerialLogHandler *)_handler;

    return serial_tx_send(handler->serial_tx, src, amount);
}

void serial_log_handler_init(SerialLogHandler *self, SerialTx *serial_tx)
{
    log_handler_init_funcs(&self->handler, serial_handler_write, NULL);
    self->serial_tx = serial_tx;
}
