#include "ear_saver.h"
#include "project_config.h"
#include "project.h"

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
