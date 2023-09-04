#pragma once

#include "usb_audio.h"

#define USBFS_EP_1_ISR_ENTRY_CALLBACK
#define USBFS_EP_1_ISR_EntryCallback() usb_audio_out_ep_isr()

#define USBFS_EP_3_ISR_ENTRY_CALLBACK
#define USBFS_EP_3_ISR_EntryCallback() usb_audio_out_fb_isr()
