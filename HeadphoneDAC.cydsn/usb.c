#include "usb.h"
#include "audio_out.h"
#include "project.h"

volatile uint8_t fb_data[3];

// Default sample rate. 48kHz. Overridden at usb_start().
volatile uint32_t sample_rate_feedback = 0x0C0000;

// Audio output data is dumped into here.
uint8_t usb_audio_out_buf[USB_MAX_BUF_SIZE];

uint8_t usb_status = 0;
uint8_t usb_alt_setting[USB_NO_STREAM_IFACE] = {0xFF, 0xFF};

static uint32_t sample_rate = 48000;

// Sample rate converted to the usb feedback register format.
static inline uint32_t ftofb(uint32_t _sample_rate)
{
    return (16384 * _sample_rate) / 1000;
}

void usb_start(uint32_t _sample_rate)
{
    // k * 48e3 = 0x0C0000. k = 16.384
    // feedback_reg = (16384 * sample_rate) / 1000
    sample_rate = _sample_rate;
    sample_rate_feedback = ftofb(_sample_rate);

    fb_data[2] = (sample_rate_feedback >> 16) & 0xFF;
    fb_data[1] = (sample_rate_feedback >> 8) & 0xFF;
    fb_data[0] = sample_rate_feedback & 0xFF;

    // Initialize the output buffer
    for (size_t i = 0; i < USB_MAX_BUF_SIZE; i++)
    {
        usb_audio_out_buf[i] = 0;
    }

    usb_status = 0;
    // Start and enumerate USB.
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration())
        ;
}

// This is called whenever the host requests feedback. Not sure what we need to do here.
void usb_feedback(void)
{
    // Load new feedback into ep. This is calculated by measuring the bus clock relative to the frame clock.
    fb_data[2] = (sample_rate_feedback >> 16) & 0xFF;
    fb_data[1] = (sample_rate_feedback >> 8) & 0xFF;
    fb_data[0] = sample_rate_feedback & 0xFF;

    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY))
    {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
}
// Fun fun usb stuff.
void usb_service(void)
{
    uint16_t i;
    if (usb_status == USB_STS_INACTIVE)
    {
        usb_status = USB_STS_INIT;
    }
    if (usb_status == USB_STS_INIT)
    {
        usb_status = USB_STS_ENUM;
        // Initialize buffers.
        for (i = 0; i < USB_MAX_BUF_SIZE; i++)
        {
            usb_audio_out_buf[i] = 0u;
        }
    }

    if (USBFS_IsConfigurationChanged())
    {
        if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USBFS_GetInterfaceSetting(1))
        {
            usb_alt_setting[USB_OUT_IFACE_INDEX] = USBFS_GetInterfaceSetting(1);
            audio_out_disable();
            I2S_Stop();
            I2S_Start();
            if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USB_ALT_ZEROBW)
            {
                USBFS_ReadOutEP(AUDIO_OUT_EP, usb_audio_out_buf, USB_MAX_BUF_SIZE);
                USBFS_EnableOutEP(AUDIO_OUT_EP);
                USBFS_LoadInEP(AUDIO_FB_EP, (const uint8_t *)fb_data, 3);
                USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
            }
            sample_rate_feedback = ftofb(sample_rate);
            fb_data[2] = (sample_rate_feedback >> 16) & 0xFF;
            fb_data[1] = (sample_rate_feedback >> 8) & 0xFF;
            fb_data[0] = sample_rate_feedback & 0xFF;
        }
        if (usb_alt_setting[USB_IN_IFACE_INDEX] != USBFS_GetInterfaceSetting(2))
        {
            usb_alt_setting[USB_IN_IFACE_INDEX] = USBFS_GetInterfaceSetting(2);
            // Audio in stuff.
        }
    }
}
