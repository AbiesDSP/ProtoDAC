#include "usb_serial_logger.h"
#include "usb.h"

#include "project_config.h"

int usb_serial_handler_write(LogHandler *_handler, const char *src, int amount)
{
    (void)_handler;
    int written = 0;

    written = usb_serial_write(src, amount);

    return written;
}

LogHandler usb_serial_log_handler = {
    .write = usb_serial_handler_write,
    .read = 0,
};
