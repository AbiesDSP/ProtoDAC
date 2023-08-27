#pragma once
#include "cptr.h"
#include <cytypes.h>

#include "project_config.h"

void serial_tx_init(void);
int serial_send(const void *src, int amount);
int serial_tx_buffer_size(void);
int serial_tx_free_size(void);

// Update the buffer size when a transaction completes,
// And send any messages waiting in the buffer.
void SerialSender(void *pvParameters);
