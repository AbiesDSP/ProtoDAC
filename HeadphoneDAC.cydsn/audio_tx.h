#pragma once
#include <stdint.h>

// Indicates that i2s is enabled and transmitting data. If it has underflowed, this will be cleared.
#define AUDIO_OUT_STS_ACTIVE 0x01
// Transmitter is in the overflow state.
#define AUDIO_OUT_STS_OVERFLOW 0x02

// Start up DMA channels, I2S, and ISRs.
void audio_tx_init(void);
// Get current size of the tx buffer.
int audio_tx_size(void);
// Audio tx status indicates if audio is off, active, or overflowed.
uint8_t audio_tx_status(void);

/* Audio Tx Tasks. Free RTOS Tasks. */
/*
    AudioTx will write data to the byte swap component, which gets sent to the main i2s buffer.
    Notify the AudioTx with a task notification when data is available.
    xTaskNotifyFromISR(xAudioOutTask, audio_out_count, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
*/
void AudioTx(void *source_buffer);

/* AudioTxMonitor will monitor the audio tx buffer size to
 * determine whether to enable or disable audio output.
 * When audio is streaming in and the buffer is half full it enabled.
 * It checks for overflow, underflow, and handles it as gracefully as it can
 * by dropping data or disabling i2s_tx.
 */
void AudioTxMonitor(void *pvParameters);

/* Low priority logging to monitor the buffer size. */
void AudioTxLogging(void *pvParameters);

// Start and stop audio playback and I2S.
void audio_tx_enable(void);
void audio_tx_disable(void);
