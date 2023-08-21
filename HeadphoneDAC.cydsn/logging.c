#include "logging.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "cptr.h"
#include "balloc.h"

const char *log_level_strs[] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
};

int log_handler_init_funcs(log_handler *handler, log_handler_write_func write, log_handler_read_func read)
{
    handler->write = write;
    handler->read = read;
    
    return 0;
}

int stdout_handler(log_handler *_self, const char *src, int size)
{
    return printf(src);
}

const log_handler _stdout_log_handler = {.write = stdout_handler, .read = NULL};
const log_handler *stdout_log_handler = &_stdout_log_handler;

int mem_write_handler_func(log_handler *_self, const char *src, int size)
{
    int error = 0;
    mem_log_handler *self = (mem_log_handler *)_self;

    cptr_copy_into(&self->buf, src, size);

    return error;
}

int mem_read_handler_func(log_handler *_self, char *dst, int size)
{
    int error = 0;
    mem_log_handler *self = (mem_log_handler *)_self;

    cptr_copy_from(&self->buf, dst, size);

    return error;
}

void mem_log_handler_init(mem_log_handler *handler, uint8_t *buf, int size)
{
    cptr_init(&handler->buf, buf, size);
    log_handler_init_funcs(&handler->base, mem_write_handler_func, mem_read_handler_func);
}

/*  */
int sprintf_formatter(const log_formatter *formatter, char *dst, const log_entry_config *config, va_list args);
int raw_formatter(const log_formatter *formatter, char *dst, const log_entry_config *config, va_list args);

const log_formatter _default_log_formatter = {.format = sprintf_formatter, .header_fmt = "%s: ", .args_fmt = "%s", .delim = 0};
const log_formatter *default_log_formatter = &_default_log_formatter;

void logger_init(logger *log, const log_handler *handler, const log_formatter *formatter)
{
    list_init(&log->handlers);
    if (handler != NULL)
    {
        list_append(&log->handlers, handler);
    }
    if (formatter == NULL)
    {
        formatter = default_log_formatter;
    }

    log->formatter = formatter;
    log->level = LOG_ERROR;
}

void log_(const logger *log, const log_entry_config *config, va_list args)
{
    if (config->level >= log->level)
    {
        // Allocate a temporary block of memory for processing the message.
        // char *message_buf = balloc_bigblock();
        char *message_buf = balloc(BALLOC_BLOCK_SIZE);

        // Format into the message buffer.
        int amount = log->formatter->format(log->formatter, message_buf, config, args);

        log_handler *handler;
        for (list_node *it = log->handlers.begin; it != NULL; it = it->next)
        {
            handler = (log_handler *)it->ref;
            handler->write(handler, message_buf, amount);
        }

        // Release the buffer.
        bfree(message_buf);
    }
}

/*  */
int sprintf_formatter(const log_formatter *self, char *dst, const log_entry_config *config, va_list args)
{
    char *dptr = dst;
    const char esc_char = 27;

    // Prepend the log level.
    dptr += sprintf(dptr, self->header_fmt, log_level2str(config->level));
    dptr += vsprintf(dptr, self->args_fmt, args);

    // Use esc_char delim to
    if (self->delim != esc_char)
    {
        *dptr = self->delim;
        dptr++;
    }

    return dptr - dst;
}

void sprintf_formatter_init(log_formatter *formatter, const char *header_fmt, const char *args_fmt)
{
    formatter->format = sprintf_formatter;
    formatter->header_fmt = header_fmt;
    formatter->args_fmt = args_fmt;
    formatter->delim = default_log_formatter->delim;
}

int raw_formatter(const log_formatter *formatter, char *dst, const log_entry_config *config, va_list args)
{
    return vsprintf(dst, "%s", args);
}