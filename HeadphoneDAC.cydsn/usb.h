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
// 48kHz
// #define FB_DEFAULT 0x0C0000
// 96kHz
// #define FB_DEFAULT 0x180000
#define FS48KHZ 48000
#define FS96KHZ 96000

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

// Asynchronous feedback register. Update this based on measurements of the bus clock.
extern volatile uint32_t sample_rate_feedback;

extern uint8_t usb_status;
extern uint8_t usb_alt_setting[USB_NO_STREAM_IFACE];

extern uint8_t usb_audio_out_buf[USB_MAX_BUF_SIZE];

// Set up USB and which buffer audio output gets put into.
void usb_start(uint32_t sample_rate);
// Called every 128 samples when feedback ep is updated. Put this in cyapicallbacks as USBFS EP3 Entry Callback.
void usb_feedback(void);
// Call in main loop to handle usb stuff
void usb_service(void);
