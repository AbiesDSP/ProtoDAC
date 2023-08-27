#include "serial_logger.h"
#include "serial_tx.h"

#include "project_config.h"

int serial_handler_write(LogHandler *_handler, const char *src, int amount)
{
    (void)_handler;
    int written = 0;
    
    written = serial_send(src, amount);

    
    return written;
}

void SerialLogger(void *pvParameters)
{
}

Logger serial_log;
LogHandler serial_log_handler = {
    .write = serial_handler_write,
    .read = NULL,
};
