#include "project_config.h"

#include "usb_serial.h"
#include "usb.h"
#include "usb_pvt.h"
#include "cptr.h"

#include "command.h"
#include "booter.h"

static TaskHandle_t SerialTxTask = NULL;
static SemaphoreHandle_t tx_buf_lock = NULL;

uint8_t usb_serial_tx_ep_buf[USB_SERIAL_EP_BUF_SIZE];
uint8_t usb_serial_rx_ep_buf[USB_SERIAL_EP_BUF_SIZE];

// Larger circular receive buffer.
static uint8_t serial_tx_buf[USB_SERIAL_TX_BUF_SIZE];
static int serial_tx_buf_size = 0;
static Cptr serial_tx_write_ptr;
static Cptr serial_tx_read_ptr;

void usb_serial_init(void)
{
    tx_buf_lock = xSemaphoreCreateMutex();
    cptr_init(&serial_tx_write_ptr, serial_tx_buf, USB_SERIAL_TX_BUF_SIZE);
    cptr_init(&serial_tx_read_ptr, serial_tx_buf, USB_SERIAL_TX_BUF_SIZE);
    serial_tx_buf_size = 0;
}

int usb_serial_tx_buf_size(void)
{
    return serial_tx_buf_size;
}

int usb_serial_write(const void *src, int amount)
{
    if (xSemaphoreTake(tx_buf_lock, pdMS_TO_TICKS(10)))
    {
        cptr_copy_into(&serial_tx_write_ptr, src, amount);
        serial_tx_buf_size += amount;
        xSemaphoreGive(tx_buf_lock);
        xTaskNotifyGive(SerialTxTask);
        return amount;
    }
    return 0;
}

void USBSerialTx(void *pvParameters)
{
    (void)pvParameters;

    SerialTxTask = xTaskGetCurrentTaskHandle();

    while (!usb_is_configured())
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    for (ever)
    {
        // Wait for more data.
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2)))
        {
        }

        // timed out or more data.
        int copy_amount = usb_serial_tx_buf_size(); // atomic copy.
        copy_amount = copy_amount > USB_SERIAL_EP_BUF_SIZE ? USB_SERIAL_EP_BUF_SIZE : copy_amount;

        // Send the data.
        if (copy_amount && USBFS_GetEPState(USB_SERIAL_TX_EP) == USBFS_IN_BUFFER_EMPTY)
        {
            if (xSemaphoreTake(tx_buf_lock, pdMS_TO_TICKS(10)))
            {
                cptr_copy_from(&serial_tx_read_ptr, usb_serial_tx_ep_buf, copy_amount);
                serial_tx_buf_size -= copy_amount;
                xSemaphoreGive(tx_buf_lock);
                USBFS_LoadEP(USB_SERIAL_TX_EP, NULL, copy_amount);
            }
        }
    }
}

void USBSerialRx(void *pvParameters)
{
    (void)pvParameters;

    while (!usb_is_configured())
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    for (ever)
    {
        vTaskDelay(pdMS_TO_TICKS(2));
        // int count = USBFS_GetEPCount(USB_SERIAL_RX_EP);
        if (USBFS_GetEPState(USB_SERIAL_RX_EP))
        {
            int count = USBFS_GetEPCount(USB_SERIAL_RX_EP);

            Command *cmd = (Command *)usb_serial_rx_ep_buf;
            if (cmd->address == COMMAND_ENTER_BOOTLOAD_ADDR)
            {
                enter_bootload();
            }

            USBFS_EnableOutEP(USB_SERIAL_RX_EP);
        }
    }
}
