#pragma once

#include "cytypes.h"
#include <stdint.h>

//// 48kHz
// #define AUDIO_OUT_TRANSFER_SIZE 288
//// 96kHz
//// #define AUDIO_OUT_TRANSFER_SIZE (576)
#define AUDIO_OUT_TRANSFER_SIZE 512

#define AUDIO_OUT_N_TD 32
#define AUDIO_OUT_BUF_SIZE (AUDIO_OUT_N_TD * AUDIO_OUT_TRANSFER_SIZE)

#define AUDIO_OUT_LOW_LIMIT (AUDIO_OUT_TRANSFER_SIZE * 2)
#define AUDIO_OUT_HIGH_LIMIT (AUDIO_OUT_BUF_SIZE - AUDIO_OUT_TRANSFER_SIZE * 2)
#define AUDIO_OUT_ACTIVE_LIMIT (AUDIO_OUT_BUF_SIZE >> 1)

#define AUDIO_OUT_STS_ACTIVE 0x01
#define AUDIO_OUT_STS_OVERFLOW 0x02
#define AUDIO_OUT_STS_UNDERFLOW 0x04
#define AUDIO_OUT_STS_RESET 0x08u

// The number of bytes in the audio_out_process buffer.
extern uint16_t audio_out_count;

// This is the main output buffer that the I2S DMA will read from
extern uint8_t audio_out_buf[AUDIO_OUT_BUF_SIZE];

// Current size of the main output buffer.
extern volatile uint16_t audio_out_buffer_size;

// Audio is currently playing.
extern volatile uint8_t audio_out_active;

// Status register. Updated audiomatically.
extern volatile uint8_t audio_out_status;

extern volatile int audio_out_update_flag;

// // Initialize module with hardware configuration.
// void audio_out_init(void);

// Start up DMA channels, I2S, and ISRs.
void audio_out_start(void);

// Gets called on audio out ep isr. Put in cyapicallbacks.h for the USBFS EP1 Entry Callback.
void audio_out_update(void);

// Send processed data to bs component DMA.
void audio_out_transmit(void);

// Start and stop audio playback and I2S.
void audio_out_enable(void);
void audio_out_disable(void);

// ISRs for TopDesign peripherals.
CY_ISR_PROTO(bs_tx_done_isr);
CY_ISR_PROTO(i2s_tx_done_isr);
