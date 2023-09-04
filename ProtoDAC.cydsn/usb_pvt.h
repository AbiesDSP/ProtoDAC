#pragma once
#include "project_config.h"
#include <cytypes.h>

// USB Endpoint buffers.
extern uint8_t usb_audio_out_ep_buf[USB_AUDIO_EP_BUF_SIZE];
extern uint8_t usb_audio_in_ep_buf[USB_AUDIO_EP_BUF_SIZE];
extern uint8_t usb_audio_fb_ep_buf[3];

extern uint8_t usb_serial_tx_ep_buf[USB_SERIAL_EP_BUF_SIZE];
extern uint8_t usb_serial_rx_ep_buf[USB_SERIAL_EP_BUF_SIZE];
