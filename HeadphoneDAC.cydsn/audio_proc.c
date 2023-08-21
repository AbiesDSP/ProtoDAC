#include "audio_proc.h"

#define N_CHANNELS 2
#define NBYTES 3

float proc_buf0[AUDIO_PROC_BUF_SIZE];
float proc_buf1[AUDIO_PROC_BUF_SIZE];

static inline int32_t unpack_sample(const uint8_t *src)
{
    int32_t result = ((uint32_t)src[2] << 24) | ((uint32_t)src[1] << 16) | ((uint32_t)src[0] << 8) | 0;
    return result;
}

static inline void pack_sample(int32_t sample, uint8_t *dst)
{
    dst[2] = ((sample >> 24) & 0xFF);
    dst[1] = ((sample >> 16) & 0xFF);
    dst[0] = ((sample >> 8) & 0xFF);
}

int unpack_usb_data(const uint8_t *usb_buf, int n_samples, int32_t *dst, int channel)
{
    int offset = channel ? 3 : 0;
    const uint8_t *src = usb_buf + offset;

    for (int i = 0; i < n_samples; i++)
    {
        dst[i] = unpack_sample(src);
        src += 6;
    }

    return n_samples;
}

void pack_usb_data(uint8_t *usb_buf, int n_samples, const int32_t *src, int channel)
{
    int offset = channel ? 3 : 0;
    uint8_t *dst = usb_buf + offset;
    for (int i = 0; i < n_samples; i++)
    {
        pack_sample(src[i], dst);
        dst += 6;
    }
}

int unpack_usb_data_float(const uint8_t *usb_buf, int n_samples, float *dst, int channel)
{
    int offset = channel ? 3 : 0;
    const uint8_t *src = usb_buf + offset;
    int32_t sample = 0;

    for (int i = 0; i < n_samples; i++)
    {
        sample = unpack_sample(src);
        dst[i] = (float)sample / (float)INT32_MAX;
        src += 6;
    }

    return n_samples;
}

void pack_usb_data_float(uint8_t *usb_buf, int n_samples, const float *src, int channel)
{
    int offset = channel ? 3 : 0;
    uint8_t *dst = usb_buf + offset;
    int32_t sample;
    for (int i = 0; i < n_samples; i++)
    {
        sample = src[i] * (float)INT32_MAX;
        pack_sample(sample, dst);
        dst += 6;
    }
}

void volume(const float *src, float *dst, int amount, float gain)
{
    for (int i = 0; i < amount; i++)
    {
        dst[i] = src[i] * gain;
    }
}

void lpf_exp(const float *src, float *dst, int n_samples, float a, float *last)
{
    float y = *last;
    float b = 1 - a;
    for (int i = 0; i < n_samples; i++)
    {
        y = y * a + b*src[i];
        dst[i] = y;
    }
    *last = y;
}
