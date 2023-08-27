#pragma once
#include "cptr.h"
#include <cytypes.h>

#define SERIAL_RX_OVERRUN 0x2

void serial_rx_init(void);
int serial_receive(void *dst, int amount);

int serial_rx_size(void);
int serial_rx_free_size(void);

void SerialReceiver(void *pvParameters);
