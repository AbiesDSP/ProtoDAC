#pragma once
#include "logging.h"
#include "cptr.h"

typedef struct SramLogHandler SramLogHandler;
struct SramLogHandler
{
    LogHandler base;
    Cptr buf;
};

void sram_log_handler_init(SramLogHandler *handler, uint8_t *buf, int size);
