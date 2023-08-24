#include "usb.h"
#include "audio_out.h"
#include "project.h"

volatile int usb_audio_out_update_flag = 0;
volatile int usb_audio_out_count = 0;
uint8_t usb_audio_out_buf[USB_MAX_BUF_SIZE];

volatile uint8_t fb_data[3];
static uint32_t sample_rate = 0;

uint8_t usb_status = 0;
uint8_t usb_alt_setting[USB_NO_STREAM_IFACE] = {0xFF, 0xFF};

// Sample rate converted to the usb feedback register format.
static inline uint32_t fs_to_feedback(uint32_t sample_rate)
{
    return (16384 * sample_rate) / 1000;
}

static inline void unpack_feedback(uint32_t feedback)
{
    int int_status = CyEnterCriticalSection();
    fb_data[2] = (feedback >> 16) & 0xFF;
    fb_data[1] = (feedback >> 8) & 0xFF;
    fb_data[0] = feedback & 0xFF;
    CyExitCriticalSection(int_status);
}

void usb_start(uint32_t _sample_rate)
{   
    sample_rate = _sample_rate;
    
    // Set initial feedback value.
    unpack_feedback(fs_to_feedback(sample_rate));

    usb_status = 0;
    // Start and enumerate USB.
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration())
        ;
}

CY_ISR(usb_audio_out_ep_isr)
{
    usb_audio_out_count = USBFS_GetEPCount(AUDIO_OUT_EP);
    usb_audio_out_update_flag = 1;
}

// This is called whenever the host requests feedback.
CY_ISR(usb_audio_out_fb_isr)
{
    // Load new feedback into ep.
    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY))
    {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
}
// Fun fun usb stuff.
void usb_service(void)
{
    if (usb_status == USB_STS_INACTIVE)
    {
        usb_status = USB_STS_INIT;
    }
    if (usb_status == USB_STS_INIT)
    {
        usb_status = USB_STS_ENUM;
    }

    if (USBFS_IsConfigurationChanged())
    {
        if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USBFS_GetInterfaceSetting(1))
        {
            usb_alt_setting[USB_OUT_IFACE_INDEX] = USBFS_GetInterfaceSetting(1);

            if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USB_ALT_ZEROBW)
            {
                USBFS_ReadOutEP(AUDIO_OUT_EP, usb_audio_out_buf, USB_MAX_BUF_SIZE);
                USBFS_EnableOutEP(AUDIO_OUT_EP);
                USBFS_LoadInEP(AUDIO_FB_EP, (const uint8_t *)fb_data, 3);
                USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
            }
            // Initialize feedback for new sample rate
        }
        if (usb_alt_setting[USB_IN_IFACE_INDEX] != USBFS_GetInterfaceSetting(2))
        {
            usb_alt_setting[USB_IN_IFACE_INDEX] = USBFS_GetInterfaceSetting(2);
            // Audio in stuff.
        }
    }
}

// Running average over 8 128ms captures. 1.024 seconds.
#define N_AVERAGE 8
static uint32_t rolling_average_buf[N_AVERAGE];
static uint32_t running_sum = 0;
static int feedback_iter = 0;

uint32_t usb_update_feedback(uint32_t feedback)
{   
    running_sum += (feedback - rolling_average_buf[feedback_iter]);
    rolling_average_buf[feedback_iter++] = feedback;
    
    if (feedback_iter == N_AVERAGE)
    {
        feedback_iter = 0;
        
        uint32_t rolling_average = running_sum >> 4;
        unpack_feedback(rolling_average);
        
        return rolling_average;
    }
    
    return 0;
}
