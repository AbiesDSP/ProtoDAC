#pragma once
#include "cptr.h"
#include <cytypes.h>

#include "project_config.h"

extern SemaphoreHandle_t serial_tx_lock;

void serial_tx_init(void);
int serial_send(const void *src, int amount);
int serial_tx_buffer_size(void);
int serial_tx_free_size(void);

void SerialSender(void *pvParameters);
