#pragma once
#include <stdint.h>

// Pre-allocated space for i2s tds.
#define AUDIO_OUT_MAX_TDS 64

// Indicates that i2s is enabled and transmitting data. If it has underflowed, this will be cleared.
#define AUDIO_OUT_STS_ACTIVE 0x01
// Transmitter is in the overflow state.
#define AUDIO_OUT_STS_OVERFLOW 0x02

// Configure audio output.
typedef struct AudioTxConfig AudioTxConfig;
struct AudioTxConfig
{
    // Size of dma transactions. audio_out_buf_size updates in this increment size.
    int transfer_size;
    int n_tds;
    // Set to null to use malloc, otherwise the size must be transfer_size*n_tds.
    const uint8_t *buffer;
    // Number of blocks(transfer_size) before limit before under/overflow is detected.
    int overflow_limit;
};

// Current size of the main output buffer.
extern volatile int audio_out_buffer_size;

// Status register. Indicates if audio is playing
extern volatile uint8_t audio_out_status;

// Start up DMA channels, I2S, and ISRs.
void audio_out_init(const AudioTxConfig *config);

// Call this to write data to the transmit buffer.
void audio_out_transmit(const uint8_t *source_data, int amount);

// Start and stop audio playback and I2S.
void audio_out_enable(void);

void audio_out_disable(void);
