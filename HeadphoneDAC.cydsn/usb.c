#include "usb.h"
#include "project_config.h"
#include "audio_tx.h"
#include <USBFS.h>

uint8_t _usb_audio_out_buf[USB_MAX_BUF_SIZE];
const uint8_t *usb_audio_out_buf = _usb_audio_out_buf;

static uint32_t sample_rate = 0;

static TaskHandle_t xAudioOutTask = NULL;
static TaskHandle_t xUSBServiceAudioFeedbackEp = NULL;

// Sample rate converted to the usb feedback register format.
static inline uint32_t fs_to_feedback(uint32_t sample_rate)
{
    return (16384 * sample_rate) / 1000;
}
static uint32_t new_feedback = 0;
static uint8_t fb_data[3];

void usb_init(void)
{
    sample_rate = AUDIO_SAMPLE_RATE;
    new_feedback = fs_to_feedback(sample_rate);
    fb_data[2] = (new_feedback >> 16) & 0xFF;
    fb_data[1] = (new_feedback >> 8) & 0xFF;
    fb_data[0] = new_feedback & 0xFF;
}

void usb_set_audio_output_task(void *_xAudioOutTask)
{
    xAudioOutTask = (TaskHandle_t)_xAudioOutTask;
}

void usb_update_feedback(uint32_t feedback_update)
{
    new_feedback = feedback_update;
}

void USBServiceAudioFeedbackEp(void *pvParameters)
{
    (void)pvParameters;

    xUSBServiceAudioFeedbackEp = xTaskGetCurrentTaskHandle();

    uint32_t _feedback = 0;
    uint32_t notifications = 0;
    const TickType_t xMaxWait = pdMS_TO_TICKS(60);

    for (ever)
    {
        notifications = ulTaskNotifyTake(pdTRUE, xMaxWait);
        if (notifications)
        {
            // Update the feedback value.
            _feedback = new_feedback;
            fb_data[2] = (_feedback >> 16) & 0xFF;
            fb_data[1] = (_feedback >> 8) & 0xFF;
            fb_data[0] = _feedback & 0xFF;

            // Load new feedback into ep.
            if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY))
            {
                USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
            }
        }
    }
}

CY_ISR(usb_audio_out_ep_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t audio_out_count = USBFS_GetEPCount(AUDIO_OUT_EP);

    // Inform the audio out task there is new data.
    xTaskNotifyFromISR(xAudioOutTask, audio_out_count, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// This is called whenever the host requests feedback.
CY_ISR(usb_audio_out_fb_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(xUSBServiceAudioFeedbackEp, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Fun fun usb stuff.
void USBConfigService(void *pvParameters)
{
    (void)pvParameters;

    uint8_t usb_alt_setting[USB_NO_STREAM_IFACE] = {0xFF, 0xFF};
    const TickType_t xRefreshDelay = pdMS_TO_TICKS(5);
    
    // Start and enumerate USB.
    //const TickType_t xStartupDelay = pdMS_TO_TICKS(100);
    //vTaskDelay(xStartupDelay);
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration());
    
    for (ever)
    {
        if (USBFS_IsConfigurationChanged())
        {
            if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USBFS_GetInterfaceSetting(1))
            {
                usb_alt_setting[USB_OUT_IFACE_INDEX] = USBFS_GetInterfaceSetting(1);

                if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USB_ALT_ZEROBW)
                {
                    USBFS_ReadOutEP(AUDIO_OUT_EP, (uint8_t*)usb_audio_out_buf, USB_MAX_BUF_SIZE);
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
        vTaskDelay(xRefreshDelay);
    }
}
