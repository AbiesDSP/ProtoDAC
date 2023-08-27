#pragma once
#include <stdint.h>

// Indicates that i2s is enabled and transmitting data. If it has underflowed, this will be cleared.
#define AUDIO_OUT_STS_ACTIVE 0x01
// Transmitter is in the overflow state.
#define AUDIO_OUT_STS_OVERFLOW 0x02

// Current size of the main output buffer.

// Status register. Indicates if audio is playing
extern uint8_t audio_out_status;

// Start up DMA channels, I2S, and ISRs.
void audio_out_init(void);

// Call this to write data to the transmit buffer.
void audio_out_transmit(const uint8_t *source_data, int amount);
int audio_tx_size(void);

// Audio Tx Tasks
void AudioTransmit(void *source_buffer);
void AudioTxMonitor(void *pvParameters);
void AudioTxLogging(void *pvParameters);

// Start and stop audio playback and I2S.
void audio_out_enable(void);

void audio_out_disable(void);
