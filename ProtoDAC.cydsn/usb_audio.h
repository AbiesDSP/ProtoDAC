#pragma once
#include <cytypes.h>

void usb_audio_init(void);
void usb_set_audio_output_task(void *AudioOutTask);
uint8_t *usb_get_audio_out_ep_buf(void);

// Handle the audio out ep.
// Notify this task with an update of the feedback value.
void USBServiceAudioFeedbackEp(void *pvParameters);
void usb_update_audio_fb(uint32_t feedback);

// USB will initialize and use these with cyapicallbacks.h
CY_ISR_PROTO(usb_audio_out_ep_isr);
CY_ISR_PROTO(usb_audio_out_fb_isr);
