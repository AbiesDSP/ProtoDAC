#include "project_config.h"
#include "ear_saver.h"
#include "project.h"

void ear_saver_mute(void)
{
    MuteControl_Write(0);
}

void ear_saver_unmute(void)
{
    MuteControl_Write(1);
}

void EarSaver(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t xStartupDelay = pdMS_TO_TICKS(EAR_SAVER_STARTUP_DELAY);
    const TickType_t xTaskDelay = pdMS_TO_TICKS(EAR_SAVER_RESET_INTERVAL);

    vTaskDelay(xStartupDelay);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    EarSaverTimer_Start();

    for (ever)
    {
        vTaskDelayUntil(&xLastWakeTime, xTaskDelay);
        EarSaverReset_Write(1);
    }
}
