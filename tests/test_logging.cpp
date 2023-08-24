extern "C"
{
#include "logging.h"
}
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Logging Basics", "[.][logging_basic]")
{
    Logger log;
    logger_init(&log, stdout_log_handler, default_log_formatter);

    log.level = LOG_TRACE;

    log_trace(&log, "Some Trace Messagexxxxxxxxxxxxxxxxxxxx\n");
    log_debug(&log, "Some Debug Message\n");
    log_info(&log, "Some Info Message\n");
    log_warn(&log, "Some Warn Message\n");
    log_error(&log, "Some Error Message\n", "Extra");
}

TEST_CASE("Logging Memory", "[logging]")
{
    const int some_mem_size = 64;
    static uint8_t some_mem[some_mem_size];
    for (int i = 0; i < some_mem_size; i++)
    {
        some_mem[i] = 0;
    }

    SramLogHandler some_mem_handler;
    sram_log_handler_init(&some_mem_handler, some_mem, some_mem_size);

    LogFormatter formatter;
    sprintf_formatter_init(&formatter, "%s: ");

    Logger log;
    logger_init(&log, &some_mem_handler.base, &formatter);

    log.level = LOG_TRACE;

    int some_data = 42;
    log_trace(&log, "%d", some_data);
    // printf((char *)some_mem);
    log_debug(&log, "%d", some_data);
    log_info(&log, "%d", some_data);
    log_warn(&log, "%d", some_data);
    log_error(&log, "%d", some_data, "Extra");

    // Print it?
    printf((char *)some_mem);
}
