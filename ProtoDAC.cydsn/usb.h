#pragma once
#include "usb_audio.h"
#include "usb_serial.h"

// Set up USB.
int usb_is_configured(void);

// Handle usb configuration when it changes.
void USBConfigService(void *pvParameters);
