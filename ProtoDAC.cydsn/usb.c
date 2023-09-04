#include "project_config.h"
#include "usb.h"
#include "usb_pvt.h"
#include <USBFS.h>
#include <USBFS_cdc.h>

static uint8_t is_configured = 0;

int usb_is_configured(void)
{
    return is_configured;
}

static uint8_t audio_out_alt_setting = USB_ALT_INVALID;
static uint8_t audio_in_alt_setting = USB_ALT_INVALID;
static uint8_t serial_alt_setting = USB_ALT_INVALID;

// Fun fun usb stuff.
void USBConfigService(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t RefreshDelay = pdMS_TO_TICKS(USB_CONFIG_SERVICE_MAX_WAIT);

    // Start and enumerate USB.
    USBFS_Start(USBFS_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (!USBFS_GetConfiguration())
        ;

    USBFS_CDC_Init();
    is_configured = 1;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (ever)
    {
        if (USBFS_IsConfigurationChanged())
        {
            // Audio out changed.
            if (audio_out_alt_setting != USBFS_GetInterfaceSetting(USB_AUDIO_OUT_IFACE))
            {
                audio_out_alt_setting = USBFS_GetInterfaceSetting(USB_AUDIO_OUT_IFACE);

                if (audio_out_alt_setting != USB_AUDIO_OUT_ALT_ZEROBW)
                {
                    USBFS_ReadOutEP(USB_AUDIO_OUT_EP, usb_audio_out_ep_buf, USB_AUDIO_EP_BUF_SIZE);
                    USBFS_EnableOutEP(USB_AUDIO_OUT_EP);
                    USBFS_LoadInEP(USB_AUDIO_FB_EP, usb_audio_fb_ep_buf, 3);
                    USBFS_LoadInEP(USB_AUDIO_FB_EP, NULL, 3);
                }
                // Initialize feedback for new sample rate
            }
            if (audio_in_alt_setting != USBFS_GetInterfaceSetting(2))
            {
                audio_in_alt_setting = USBFS_GetInterfaceSetting(2);
                // Audio in stuff.
            }
            // USBUART stuff
            if (serial_alt_setting != USBFS_GetInterfaceSetting(USB_SERIAL_DATA_IFACE))
            {
                serial_alt_setting = USBFS_GetInterfaceSetting(USB_SERIAL_DATA_IFACE);
                USBFS_LoadInEP(USB_SERIAL_TX_EP, usb_serial_tx_ep_buf, USB_SERIAL_EP_BUF_SIZE);
                USBFS_ReadOutEP(USB_SERIAL_RX_EP, usb_serial_rx_ep_buf, USB_SERIAL_EP_BUF_SIZE);
                USBFS_EnableOutEP(USB_SERIAL_RX_EP);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, RefreshDelay);
    }
}