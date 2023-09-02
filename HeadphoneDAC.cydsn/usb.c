#include "usb.h"
#include "project_config.h"
#include "audio_tx.h"
#include <USBFS.h>
#include <USBFS_cdc.h>

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
    const TickType_t xMaxWait = pdMS_TO_TICKS(USB_FEEDBACK_MAX_WAIT);
    
    for (ever)
    {
        if (ulTaskNotifyTake(pdTRUE, xMaxWait))
        {
            // Update the feedback value.
            _feedback = new_feedback;
            fb_data[2] = (_feedback >> 16) & 0xFF;
            fb_data[1] = (_feedback >> 8) & 0xFF;
            fb_data[0] = _feedback & 0xFF;

            // Load new feedback into ep.
            if ((USBFS_GetEPState(USB_AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY))
            {
                USBFS_LoadInEP(USB_AUDIO_FB_EP, USBFS_NULL, 3);
            }
        }
    }
}

static uint8_t serial_tx_buf[USB_SERIAL_BUF_SIZE];
static uint8_t serial_rx_buf[USB_SERIAL_BUF_SIZE];

// Fun fun usb stuff.
void USBConfigService(void *pvParameters)
{
    (void)pvParameters;

    uint8_t audio_alt_setting[USB_AUDIO_OUT_N_ALT] = {USB_ALT_INVALID, USB_ALT_INVALID};
    uint8_t serial_alt_setting[1] = {USB_ALT_INVALID};
    
    const TickType_t xRefreshDelay = pdMS_TO_TICKS(USB_CONFIG_SERVICE_MAX_WAIT);
    
    // Start and enumerate USB.
    USBFS_Start(USBFS_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (!USBFS_GetConfiguration())
        ;
    
    USBFS_CDC_Init();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for (ever)
    {
        if (USBFS_IsConfigurationChanged())
        {
            // Audio out changed.
            if (audio_alt_setting[0] != USBFS_GetInterfaceSetting(1))
            {
                audio_alt_setting[0] = USBFS_GetInterfaceSetting(1);

                if (audio_alt_setting[0] != USB_AUDIO_OUT_ALT_ZEROBW)
                {
                    USBFS_ReadOutEP(USB_AUDIO_OUT_EP, (uint8_t *)usb_audio_out_buf, USB_MAX_BUF_SIZE);
                    USBFS_EnableOutEP(USB_AUDIO_OUT_EP);
                    USBFS_LoadInEP(USB_AUDIO_FB_EP, (const uint8_t *)fb_data, 3);
                    USBFS_LoadInEP(USB_AUDIO_FB_EP, USBFS_NULL, 3);
                }
                // Initialize feedback for new sample rate
            }
            if (audio_alt_setting[1] != USBFS_GetInterfaceSetting(2))
            {
                audio_alt_setting[1] = USBFS_GetInterfaceSetting(2);
                // Audio in stuff.
            }
            // USBUART stuff
            if (serial_alt_setting[0] != USBFS_GetInterfaceSetting(USB_SERIAL_DATA_IFACE))
            {
                serial_alt_setting[0] = USBFS_GetInterfaceSetting(USB_SERIAL_DATA_IFACE);
                USBFS_LoadInEP(USB_SERIAL_TX_EP, serial_tx_buf, USB_SERIAL_BUF_SIZE);
                USBFS_ReadOutEP(USB_SERIAL_RX_EP, serial_rx_buf, USB_SERIAL_BUF_SIZE);
                USBFS_EnableOutEP(USB_SERIAL_RX_EP);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xRefreshDelay);
        if(USBFS_GetEPState(USB_SERIAL_TX_EP)==USBFS_IN_BUFFER_EMPTY)
        {
            strcpy((char*)serial_tx_buf, "Hello, World!\n");
            USBFS_LoadEP(7,NULL,14);
        }
    }
}

CY_ISR(usb_audio_out_ep_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Get the number of bytes transferred.
    uint32_t audio_out_count = USBFS_GetEPCount(USB_AUDIO_OUT_EP);

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
