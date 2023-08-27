#pragma once
#include <cytypes.h>

#define USBFS_AUDIO_DEVICE 0
#define AUDIO_INTERFACE 1
#define AUDIO_OUT_EP 1
#define AUDIO_IN_EP 2
#define AUDIO_FB_EP 3

#define USB_STS_INACTIVE 0x00
#define USB_STS_INIT 0x01
#define USB_STS_ENUM 0x02

#define USB_NO_STREAM_IFACE 2
#define USB_OUT_IFACE_INDEX 0
#define USB_IN_IFACE_INDEX 1
#define USB_ALT_ZEROBW 0
#define USB_ALT_ACTIVE_24 2
#define USB_ALT_ACTIVE_16 1
#define USB_ALT_INVALID 0xFF

// extern uint8_t usb_alt_setting[USB_NO_STREAM_IFACE];

// extern volatile int usb_audio_out_update_flag;
// extern volatile int usb_audio_out_count;
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

void usb_audio_out_ep_isr();
void usb_audio_out_fb_isr();