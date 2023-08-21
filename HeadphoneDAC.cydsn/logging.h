#pragma once
#include "list.h"
#include "cptr.h"

#include <stdarg.h>

enum log_level
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

extern const char *log_level_strs[];
static inline const char *log_level2str(enum log_level level)
{
    return log_level_strs[level];
}

typedef struct logger logger;

typedef struct log_handler log_handler;
// Log handlers will write/read the formatted message to the physical interface. ram, stdout, etc.
typedef int (*log_handler_write_func)(log_handler *self, const char *src, int size);
typedef int (*log_handler_read_func)(log_handler *self, char *dst, int size);

struct log_handler
{
    log_handler_write_func write;
    log_handler_read_func read;
};

int log_handler_init_funcs(log_handler *handler, log_handler_write_func write, log_handler_read_func read);

// Uses printf to print log messages.
extern const log_handler *stdout_log_handler;

typedef struct mem_log_handler mem_log_handler;
struct mem_log_handler
{
    log_handler base;
    cptr buf;
};
void mem_log_handler_init(mem_log_handler *handler, uint8_t *buf, int size);

typedef struct log_entry_config log_entry_config;
struct log_entry_config
{
    enum log_level level;
    const char *source_file;
    // uint32_t timestamp;
    const char *args_fmt;
};

typedef struct log_formatter log_formatter;
// Log formatters will reformat the input data into a char* or uint8_t* array.
// They can append a header, log level, timestamp, etc. And can work with specific formats.
typedef int (*log_formatter_func)(const log_formatter *self, char *dst, const log_entry_config *config, va_list args);

struct log_formatter
{
    log_formatter_func format;
    const char *header_fmt;
};

// sprintf_formatter, header_fmt = "[%s : %s] - "
extern const log_formatter *default_log_formatter;

/* Formatters */
void sprintf_formatter_init(log_formatter *formatter, const char *header_fmt);

struct logger
{
    list handlers;
    const log_formatter *formatter;
    enum log_level level;
};

void logger_init(logger *log, const log_handler *handler, const log_formatter *formatter);
static inline int logger_add_handler(logger *log, const log_handler *handler)
{
    return list_append(&log->handlers, handler);
}

void log_(const logger *log, const log_entry_config *config, va_list args);

#if LOG_ENABLE_TRACE || _DEBUG
static inline void log_trace(const logger *log, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    log_entry_config config = {.level = LOG_TRACE, .args_fmt = args_fmt, .source_file = __FILE__};
    log_(log, &config, args);
    va_end(args);
}
#else
static inline void log_trace(const logger *log, ...)
{
    (void)log;
}
#endif

#define log_debug(log, args_fmt, ...) _log_debug (log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_debug(const logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    log_entry_config config = {
        .level = LOG_DEBUG,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_info(log, args_fmt, ...) _log_info (log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_info(const logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    log_entry_config config = {
        .level = LOG_INFO,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_warn(log, args_fmt, ...) _log_warn (log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_warn(const logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    log_entry_config config = {
        .level = LOG_WARN,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

#define log_error(log, args_fmt, ...) _log_error(log, __FILE__, args_fmt, ##__VA_ARGS__)
static inline void _log_error(const logger *log, const char *source_file, const char *args_fmt, ...)
{
    va_list args;
    va_start(args, args_fmt);

    log_entry_config config = {
        .level = LOG_ERROR,
        .source_file = source_file,
        .args_fmt = args_fmt,
    };
    log_(log, &config, args);

    va_end(args);
}

