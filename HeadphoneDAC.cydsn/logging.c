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

int log_handler_init_funcs(LogHandler *handler, LogHandlerWrite write, LogHandlerRead read)
{
    handler->write = write;
    handler->read = read;

    return 0;
}

int stdout_handler(LogHandler *_self, const char *src, int size)
{
    (void)_self;
    (void)size;
    return printf(src);
}

const LogHandler _stdout_log_handler = {.write = stdout_handler, .read = NULL};
const LogHandler *stdout_log_handler = &_stdout_log_handler;

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

/*  */
int sprintf_formatter(const LogFormatter *formatter, char *dst, const LogEntryConfig *config, va_list args);
int raw_formatter(const LogFormatter *formatter, char *dst, const LogEntryConfig *config, va_list args);

const LogFormatter _default_log_formatter = {
    .format = sprintf_formatter,
    .header_fmt = "[%s : %s] - ",
};
const LogFormatter *default_log_formatter = &_default_log_formatter;

void logger_init(Logger *log, const LogHandler *handler, const LogFormatter *formatter)
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

void log_(const Logger *log, const LogEntryConfig *config, va_list args)
{
    if (config->level >= log->level)
    {
        // Allocate a temporary block of memory for processing the message.
        // char *message_buf = balloc_bigblock();
        char *message_buf = balloc(BALLOC_BLOCK_SIZE);

        // Format into the message buffer.
        int amount = log->formatter->format(log->formatter, message_buf, config, args);

        LogHandler *handler;
        for (ListNode *it = log->handlers.begin; it != NULL; it = it->next)
        {
            handler = (LogHandler *)it->ref;
            handler->write(handler, message_buf, amount);
        }

        // Release the buffer.
        bfree(message_buf);
    }
}

/*  */
int sprintf_formatter(const LogFormatter *self, char *dst, const LogEntryConfig *config, va_list args)
{
    char *dptr = dst;
    // Prepend the log level and filename
    dptr += sprintf(dptr, self->header_fmt, log_level2str(config->level), config->source_file);
    // Timestamp?

    dptr += vsprintf(dptr, config->args_fmt, args);

    return dptr - dst;
}

void sprintf_formatter_init(LogFormatter *formatter, const char *header_fmt)
{
    formatter->format = sprintf_formatter;
    formatter->header_fmt = header_fmt;
    //    formatter->delim = default_log_formatter->delim;
}

int raw_formatter(const LogFormatter *formatter, char *dst, const LogEntryConfig *config, va_list args)
{
    (void)config;
    (void)formatter;
    return vsprintf(dst, "%s", args);
}