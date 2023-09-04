#pragma once
#include <stdint.h>

#define KNOB_VOLUME 0
#define KNOB_1 1
#define KNOB_2 2

void knobs_init(void);
float get_knob(int knob);

void KnobsUpdate(void *pvParameters);
