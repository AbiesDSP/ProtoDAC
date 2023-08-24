#pragma once
#include "list.h"
#include "cptr.h"

#include <stdarg.h>

enum LogLevel
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

extern const char *log_level_strs[];
static inline const char *log_level2str(enum LogLevel level)
{
    return log_level_strs[level];
}

// Log handlers will write/read the formatted message to the physical interface. ram, stdout, etc.
typedef struct LogHandler LogHandler;
typedef int (*LogHandlerWrite)(LogHandler *self, const char *src, int size);
typedef int (*LogHandlerRead)(LogHandler *self, char *dst, int size);

struct LogHandler
{
    LogHandlerWrite write;
    LogHandlerRead read;
};

int log_handler_init_funcs(LogHandler *handler, LogHandlerWrite write, LogHandlerRead read);

// Uses printf to print log messages.
extern const LogHandler *stdout_log_handler;

typedef struct SramLogHandler
{
    LogHandler base;
    Cptr buf;
} SramLogHandler;

void sram_log_handler_init(SramLogHandler *handler, uint8_t *buf, int size);

typedef struct LogEntryConfig
{
    enum LogLevel level;
    const char *source_file;
    // uint32_t timestamp;
    const char *args_fmt;
} LogEntryConfig;

// Log formatters will reformat the input data into a char* or uint8_t* array.
// They can append a header, log level, timestamp, etc. And can work with specific formats.
typedef struct LogFormatter LogFormatter;
typedef int (*LogFormat)(const LogFormatter *formatter, char *dst, const LogEntryConfig *config, va_list args);
struct LogFormatter
{
    LogFormat format;
    const char *header_fmt;
};

// sprintf_formatter, header_fmt = "[%s : %s] - "
extern const LogFormatter *default_log_formatter;

/* Formatters */
void sprintf_formatter_init(LogFormatter *formatter, const char *header_fmt);

typedef struct Logger
{
    List handlers;
    const LogFormatter *formatter;
    enum LogLevel level;
} Logger;

void logger_init(Logger *log, const LogHandler *handler, const LogFormatter *formatter);
static inline int logger_add_handler(Logger *log, const LogHandler *handler)
{
    return list_append(&log->handlers, handler);
}

void log_(const Logger *log, const LogEntryConfig *config, va_list args);

#if LOG_ENABLE_TRACE || _DEBUG
static inline void log_trace(const Logger *log, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    LogEntryConfig config = {
        .level = LOG_TRACE,
        .source_file = __FILE__,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);
    va_end(args);
}
#else
static inline void log_trace(const Logger *log, ...)
{
    (void)log;
}
#endif

#define log_debug(log, args_fmt, ...) _log_debug(log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_debug(const Logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    LogEntryConfig config = {
        .level = LOG_DEBUG,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_info(log, args_fmt, ...) _log_info(log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_info(const Logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    LogEntryConfig config = {
        .level = LOG_INFO,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_warn(log, args_fmt, ...) _log_warn(log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_warn(const Logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    LogEntryConfig config = {
        .level = LOG_WARN,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_error(log, args_fmt, ...) _log_error(log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_error(const Logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    LogEntryConfig config = {
        .level = LOG_ERROR,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}
