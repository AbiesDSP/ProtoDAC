#pragma once
#include "avril.h"
#include <stdint.h>

// Not-So Real Time Clock

void rtc_init(void);

extern AvrilInterface RTCIface;

void rtc_set_time(uint32_t timestamp);
uint32_t rtc_get_time(void);

void RTCUpdate(void *pvParameters);
