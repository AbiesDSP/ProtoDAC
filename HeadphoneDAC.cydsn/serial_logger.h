#pragma once
#include "logging.h"
#include "project_config.h"

extern LogHandler serial_log_handler;
extern Logger serial_log;

void SerialLogger(void *pvParameters);
