extern "C"
{
#include "logging.h"
#include "stdout_handler.h"
}
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Logging Basics", "[.][logging_basic]")
{
    Logger log;
    logger_init(&log, &stdout_log_handler, NULL, NULL, NULL);

    log.level = LOG_TRACE;

    log_trace(&log, "Some Trace Messagexxxxxxxxxxxxxxxxxxxx\n");
    log_debug(&log, "Some Debug Message\n");
    log_info(&log, "Some Info Message\n");
    log_warn(&log, "Some Warn Message\n");
    log_error(&log, "Some Error Message\n", "Extra");
}
