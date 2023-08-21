#pragma once
#include <cytypes.h>

#define AUDIO_MUTED 0
#define AUDIO_ENABLED 1

extern volatile uint8_t mute_state;

void set_mute(uint8_t state);

CY_ISR_PROTO(mute_isr);
