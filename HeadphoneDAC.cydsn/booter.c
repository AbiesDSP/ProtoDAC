#include "project_config.h"
#include "booter.h"

static int enter_flag = 0;

static int _booter_write(void *self, AvrilCommand *cmd)
{
    enter_flag = 1;
    return 0;
}

static int _booter_read(void *self, AvrilCommand *cmd)
{
}

AvrilInterface BooterIface = {
    .write = _booter_write,
    .read = _booter_read,
    .size = 4,
};

void Booter(void *pvParameters)
{
    (void)pvParameters;

    // Check the state of the button this often.
    const TickType_t RefreshDelay = pdMS_TO_TICKS(100);

    TickType_t xLastTime = xTaskGetTickCount();

    for (ever)
    {
        vTaskDelayUntil(&xLastTime, RefreshDelay);
        if (enter_flag)
        {
            MuteControl_Write(0);
            USBFS_Stop();
            // Start the bootloader.
#ifdef CY_BOOTLOADABLE_Bootloadable_H
            Bootloadable_Load();
#endif
        }
    }
}

void enter_bootload(void)
{
    enter_flag = 1;
}
