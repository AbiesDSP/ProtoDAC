#include "logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cptr.h"

#include "project_config.h"

const char *log_level_strs[] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
};
static inline const char *log_level2str(enum LogLevel level)
{
    return log_level_strs[level];
}

int log_handler_init_funcs(LogHandler *handler, LogHandlerWrite write, LogHandlerRead read)
{
    handler->write = write;
    handler->read = read;

    return 0;
}

/*  */
const char *default_header_fmt_str = "[%s : %s] - ";

void logger_init(Logger *log, const LogHandler *handler, LogHeaderFormat header_format, LogArgsFormat args_format, const char *header_fmt_str)
{
    list_init(&log->handlers);
    if (handler != NULL)
    {
        list_append(&log->handlers, handler);
    }
    if (header_format == NULL)
    {
        log->header_format = sprintf;
    }
    if (args_format == NULL)
    {
        log->args_format = vsprintf;
    }
    if (header_fmt_str == NULL)
    {
        log->header_fmt_str = default_header_fmt_str;
    }
    
    log->level = LOG_ERROR;
}

int logger_add_handler(Logger *log, const LogHandler *handler)
{
    return list_append(&log->handlers, handler);
}

// Allocate a temporary block of memory for processing the message.
static char message_buf[LOG_MESSAGE_BUF_SIZE];

void log_(const Logger *log, enum LogLevel level, const char *source_file, const char *args_fmt, ...)
{
    if (level >= GLOBAL_LOG_LEVEL && level >= log->level)
    {
        va_list args;
        va_start(args, args_fmt);
        // Format into the message buffer.
        // Format header
        // level, source_file, timestamp, 
        int message_size = log->header_format(message_buf, log->header_fmt_str, log_level2str(level), source_file);
        // Format args
        message_size += log->args_format(message_buf + message_size, args_fmt, args);

        LogHandler *handler;
        for (const ListNode *it = log->handlers.begin; it != NULL; it = it->next)
        {
            handler = (LogHandler *)it->ref;
            handler->write(handler, message_buf, message_size);
        }
        
        va_end(args);
    }
}
