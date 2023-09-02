#pragma once
#include <cytypes.h>

#define USB_STS_INACTIVE 0x00
#define USB_STS_INIT 0x01
#define USB_STS_ENUM 0x02

extern const uint8_t *usb_audio_out_buf;

// Set up USB.
void usb_init(void);
void usb_set_audio_output_task(void *xAudioOutTask);

// Handle the audio out ep.
// Handle the audio feedback ep
void USBServiceAudioFeedbackEp(void *pvParameters);

// Handle usb configuration when it changes.
void USBConfigService(void *pvParameters);

// Update feedback register with a new measured sample rate. Count the number of 12.288Mhz clock cycles in 128 usb sof pulses.
void usb_update_feedback(uint32_t feedback);

// USB will initialize and use these with cyapicallbacks.h
CY_ISR_PROTO(usb_audio_out_ep_isr);
CY_ISR_PROTO(usb_audio_out_fb_isr);
