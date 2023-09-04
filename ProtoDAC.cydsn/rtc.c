#include "project_config.h"
#include "rtc.h"
#include <project.h>

static uint32_t timestamp = 0;
static TaskHandle_t RTCTask = NULL;

CY_ISR_PROTO(rtc_inc_isr);

void rtc_init(void)
{
    rtc_isr_StartEx(rtc_inc_isr);
    RTC_Timer_Start();
}

void rtc_set_time(uint32_t _timestamp)
{
    timestamp = _timestamp;
}

uint32_t rtc_get_time(void)
{
    return timestamp;
}

int _iface_write(void *self, AvrilCommand *cmd)
{
    (void)self;
    int err = 0;
    if (cmd->amount == 4 && cmd->address == 0)
    {
        timestamp = cmd->regs[0];
    }
    else
    {
        err = -1;
    }

    return err;
}

int _iface_read(void *self, AvrilCommand *cmd)
{
    (void)self;
    int err = 0;
    if (cmd->amount == 4 && cmd->address == 0)
    {
        cmd->regs[0] = timestamp;
    }
    else
    {
        err = -1;
    }

    return err;
}

AvrilInterface RTCIface = {
    .write = _iface_write,
    .read = _iface_read,
    .size = 4,
};

void RTCUpdate(void *pvParameters)
{
    (void)pvParameters;

    RTCTask = xTaskGetCurrentTaskHandle();

    const TickType_t MaxWait = pdMS_TO_TICKS(500);

    for (ever)
    {
        if (ulTaskNotifyTake(pdTRUE, MaxWait))
        {
            timestamp++;
        }
    }
}

CY_ISR(rtc_inc_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // New update from the sync_counter. This will increment the number of notifications.
    vTaskNotifyGiveFromISR(RTCTask, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
