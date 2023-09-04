#include "project_config.h"

#include "usb_audio.h"
#include "usb_pvt.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portable.h"

uint8_t usb_audio_out_ep_buf[USB_AUDIO_EP_BUF_SIZE];
uint8_t usb_audio_fb_ep_buf[3];

TaskHandle_t USBAudioOutTask = NULL;
static TaskHandle_t USBAudioFbTask = NULL;

static uint32_t sample_rate = 0;

// Sample rate converted to the usb feedback register format.
static inline uint32_t fs_to_feedback(uint32_t sample_rate)
{
    return (16384 * sample_rate) / 1000;
}

static inline void unpack_fb(uint32_t feedback)
{
    usb_audio_fb_ep_buf[2] = (feedback >> 16) & 0xFF;
    usb_audio_fb_ep_buf[1] = (feedback >> 8) & 0xFF;
    usb_audio_fb_ep_buf[0] = feedback & 0xFF;
}

void usb_audio_init(void)
{
    sample_rate = AUDIO_SAMPLE_RATE;
    uint32_t initial_feedback = fs_to_feedback(sample_rate);
    unpack_fb(initial_feedback);
}

uint8_t *usb_get_audio_out_ep_buf(void)
{
    return usb_audio_out_ep_buf;
}

void usb_set_audio_output_task(void *AudioOutTask)
{
    USBAudioOutTask = AudioOutTask;
}

void USBServiceAudioFeedbackEp(void *pvParameters)
{
    (void)pvParameters;
    USBAudioFbTask = xTaskGetCurrentTaskHandle();

    const TickType_t MaxWait = pdMS_TO_TICKS(USB_FEEDBACK_MAX_WAIT);

    uint32_t new_feedback;

    for (ever)
    {
        if (xTaskNotifyWait(0, UINT32_MAX, &new_feedback, MaxWait))
        {
            if (new_feedback)
            {
                unpack_fb(new_feedback);
            }
            // Load new feedback into ep.
            else if ((USBFS_GetEPState(USB_AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY))
            {
                USBFS_LoadInEP(USB_AUDIO_FB_EP, USBFS_NULL, 3);
            }
        }
    }
}

CY_ISR(usb_audio_out_ep_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Get the number of bytes transferred.
    uint32_t audio_out_count = USBFS_GetEPCount(USB_AUDIO_OUT_EP);

    // Inform the audio out task there is new data.
    xTaskNotifyFromISR(USBAudioOutTask, audio_out_count, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// This is called whenever the host requests feedback.
CY_ISR(usb_audio_out_fb_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //
    xTaskNotifyFromISR(USBAudioFbTask, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
