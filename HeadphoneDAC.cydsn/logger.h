#pragma once
#include "log_level.h"
#include "list2.h"
#include <stdarg.h>

typedef struct Logger Logger;
typedef struct LogHandler LogHandler;
typedef struct LogEntryConfig LogEntryConfig;

/* Log handlers will transmit a log message to their destination interface.
 * This could be stdout, ram, a serial port, etc.
 * Derived handlers must initialize write and read.
*/

// Log write function signature. 
typedef int (*LogHandlerWrite)(LogHandler *self, const char *src, int size);
typedef int (*LogHandlerRead)(LogHandler *self, char *dst, int size);
struct LogHandler
{
    LogHandlerWrite write;
    LogHandlerRead read;
};
int log_handler_init_funcs(LogHandler *handler, LogHandlerWrite write, LogHandlerRead read);

/* Log formatters will format the input data into a char* or uint8_t* array.
 * They can append a header, log level, timestamp, etc. Or can work with specific types to avoid using format strings.
 */
// Formatters must define a format function with this signature.
typedef int (*LogHeaderFormat)(char *dst, const char *header, ...);
typedef int (*LogArgsFormat)(char *dst, const char *args_fmt, va_list args);

/* A logger object will contain a list of handlers.
 * Every log message will get sent to every handler.
 * Every log message will use the same formatter.
 * A logger 
 *
 */
struct Logger
{
    List handlers;
    enum LogLevel level;
    
    LogHeaderFormat header_format;
    LogArgsFormat args_format;
    // 
    const char *header_fmt_str;
};

// Args passed to log_. For reference only, args are passed as va_args
struct _LogHeaderArgs
{
    enum LogLevel level;
    const char *source_file;
    // uint32_t timestamp
};

void logger_init(Logger *log, const LogHandler *handler, LogHeaderFormat header_format, LogArgsFormat args_format, const char *header_fmt_str);
int logger_add_handler(Logger *log, const LogHandler *handler);

/* Generic log function. Don't call this directly.
 * The log macros will populate these fields with the needed information.
 * The macros will populate config with the name of the source file, which you can't do with a function.
*/
void log_(const Logger *log, enum LogLevel level, const char *source_file, const char *args_fmt, ...);

// Remove all trace logs from Release builds.
#ifdef DEBUG
    #define log_trace(log, args_fmt, ...) log_(log, LOG_TRACE, __FILE__, args_fmt, ##__VA_ARGS__)
#else
    #define log_trace(log, args_fmt, ...) (void)0
#endif

#define log_debug(log, args_fmt, ...) log_(log, LOG_DEBUG, __FILE__, args_fmt, ##__VA_ARGS__)
#define log_info(log, args_fmt, ...) log_(log, LOG_INFO, __FILE__, args_fmt, ##__VA_ARGS__)
#define log_warn(log, args_fmt, ...) log_(log, LOG_WARN, __FILE__, args_fmt, ##__VA_ARGS__)
#define log_error(log, args_fmt, ...) log_(log, LOG_ERROR, __FILE__, args_fmt, ##__VA_ARGS__)


// The default formatter uses the sprintf formatter.
// header_fmt = "[%s : %s] - "
// "[INFO  : logger.h] - "
