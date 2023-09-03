#pragma once
#include "avril.h"
#include <stdint.h>

// Not-So Real Time Clock

void nsrtc_init(void);

extern AvrilInterface NSRTCIface;

void nsrtc_set_time(uint32_t timestamp);
uint32_t nsrtc_get_time(void);

void NsrtcUpdate(void *pvParameters);
