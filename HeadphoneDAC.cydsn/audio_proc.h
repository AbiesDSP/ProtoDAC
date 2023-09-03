#pragma once
#include "cptr.h"

#include <stdint.h>

#define AUDIO_PROC_BUF_SIZE 192

// Two buffers for to swap data between while processing.
extern float proc_buf0[AUDIO_PROC_BUF_SIZE];
extern float proc_buf1[AUDIO_PROC_BUF_SIZE];

static inline void swap_usb_data(uint8_t *dst, const uint8_t *src, int amount)
{
    for (int i = 0; i < amount; i+= 3)
    {
        dst[i + 0] = src[i + 2];
        dst[i + 1] = src[i + 1];
        dst[i + 2] = src[i + 0];
    }
}

int unpack_usb_data(const uint8_t *usb_buf, int n_samples, int32_t *dst, int channel);
void pack_usb_data(uint8_t *usb_buf, int amount, const int32_t *src, int channel);

int unpack_usb_data_float(const uint8_t *usb_buf, int n_samples, float *dst, int channel);
void pack_usb_data_float(uint8_t *usb_buf, int n_samples, const float *src, int channel);

void pack_i2s_data_float(uint8_t *swap_buf, int n_samples, const float *src, int channel);

void volume(const float *src, float *dst, int amount, float gain);
void lpf_exp(const float *src, float *dst, int n_samples, float a, float *last);