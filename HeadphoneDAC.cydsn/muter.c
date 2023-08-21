#include "muter.h"
#include "mute.h"

volatile uint8_t mute_state = 1;

void set_mute(uint8_t state)
{
    mute_state = state;
    mute_Write(mute_state);
}

CY_ISR(mute_isr)
{
    // Toggle
    mute_state ^= 1;
    mute_Write(mute_state);
}
