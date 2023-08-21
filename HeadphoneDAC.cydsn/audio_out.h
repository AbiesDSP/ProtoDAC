#pragma once
#include <I2S.h>
#include <muter.h>
#include <stdint.h>

#define AUDIO_OUT_STS_ACTIVE 0x01
#define AUDIO_OUT_STS_OVERFLOW 0x02
#define AUDIO_OUT_STS_UNDERFLOW 0x04
#define AUDIO_OUT_STS_RESET 0x08

typedef struct audio_out_config audio_out_config;
struct audio_out_config
{
    // Size of dma transactions. audio_out_buf_size updates in this increment size.
    int transfer_size;
    int n_tds;
    // Set to null to use malloc, otherwise the size must be transfer_size*n_tds.
    const uint8_t *buffer;

    int swap_endian;
    int limit_offset;
};

// Current size of the main output buffer.
extern volatile int audio_out_buffer_size;

// Status register. Updated audiomatically.
extern volatile uint8_t audio_out_status;

// Start up DMA channels, I2S, and ISRs.
void audio_out_init(const audio_out_config *config);

// Gets called on audio out ep isr. Put in cyapicallbacks.h for the USBFS EP1 Entry Callback.
void audio_out_transmit(const uint8_t *source_data, uint16_t amount);

// Start and stop audio playback and I2S.
static inline void audio_out_enable(void)
{
    audio_out_status |= AUDIO_OUT_STS_ACTIVE;
    I2S_EnableTx();
    set_mute(AUDIO_ENABLED);
}

static inline void audio_out_disable(void)
{
    audio_out_status &= ~AUDIO_OUT_STS_ACTIVE;
    I2S_DisableTx();
    set_mute(AUDIO_MUTED);
}
