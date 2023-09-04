#include "sram_handler.h"

int sram_write_handler_func(LogHandler *_self, const char *src, int size)
{
    int error = 0;
    SramLogHandler *self = (SramLogHandler *)_self;

    cptr_copy_into(&self->buf, src, size);

    return error;
}

int sram_read_handler_func(LogHandler *_self, char *dst, int size)
{
    int error = 0;
    SramLogHandler *self = (SramLogHandler *)_self;

    cptr_copy_from(&self->buf, dst, size);

    return error;
}

void sram_log_handler_init(SramLogHandler *handler, uint8_t *buf, int size)
{
    cptr_init(&handler->buf, buf, size);
    log_handler_init_funcs(&handler->base, sram_write_handler_func, sram_read_handler_func);
}
