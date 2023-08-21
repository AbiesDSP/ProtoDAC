#pragma once
#include <stdint.h>

#define AUDIO_PROC_BUF_SIZE 192

// Two buffers for to swap data between while processing.
extern float proc_buf0[AUDIO_PROC_BUF_SIZE];
extern float proc_buf1[AUDIO_PROC_BUF_SIZE];

int unpack_usb_data(const uint8_t *usb_buf, int n_samples, int32_t *dst, int channel);
void pack_usb_data(uint8_t *usb_buf, int amount, const int32_t *src, int channel);

int unpack_usb_data_float(const uint8_t *usb_buf, int n_samples, float *dst, int channel);
void pack_usb_data_float(uint8_t *usb_buf, int n_samples, const float *src, int channel);

void volume(const float *src, float *dst, int amount, float gain);
void lpf_exp(const float *src, float *dst, int n_samples, float a, float *last);