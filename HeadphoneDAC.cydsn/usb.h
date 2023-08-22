#pragma once
#include <USBFS.h>

#define USBFS_AUDIO_DEVICE 0
#define AUDIO_INTERFACE 1
#define AUDIO_OUT_EP 1
#define AUDIO_IN_EP 2
#define AUDIO_FB_EP 3

// 48kHz
// #define USB_MAX_BUF_SIZE (294u)
// 96kHz
#define USB_MAX_BUF_SIZE 588

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

extern uint8_t usb_status;
extern uint8_t usb_alt_setting[USB_NO_STREAM_IFACE];

extern volatile int usb_audio_out_update_flag;
extern volatile int usb_audio_out_count;
extern uint8_t usb_audio_out_buf[USB_MAX_BUF_SIZE];

// Set up USB and which buffer audio output gets put into.
void usb_start(uint32_t sample_rate);

// Audio out EP update isr. Update the audio count.
CY_ISR_PROTO(usb_audio_out_ep_isr);
CY_ISR_PROTO(usb_audio_out_fb_isr);

// Call in main loop to handle usb stuff
void usb_service(void);

// Update feedback register. Count the number of 12.288Mhz clock cycles in 128 usb sof pulses.
void usb_update_feedback(uint32_t feedback);
