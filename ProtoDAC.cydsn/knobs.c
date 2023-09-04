#include "project_config.h"
#include "knobs.h"
#include "project.h"

int16_t knobs[3];

static TaskHandle_t KnobsTask = NULL;

// CY_ISR_PROTO(adc_eoc_isr);

void knobs_init(void)
{
    ADC_SAR_Seq_Start();
    KnobSourceDAC_Start();
    PGA_Start();
    // adc_sar_isr_StartEx(adc_eoc_isr);
}

float knob_scales[3] = {0.0002570694087403599, 0.0002570694087403599, 0.0002570694087403599};
int16_t knob_offsets[3] = {0, 0, 0};

float get_knob(int ch)
{
    float res = (float)(knobs[ch] - knob_offsets[ch]) * knob_scales[ch];
    return res;
}

void KnobsUpdate(void *pvParameters)
{
    (void)pvParameters;
    KnobsTask = xTaskGetCurrentTaskHandle();

    const TickType_t MaxWait = pdMS_TO_TICKS(10);
    TickType_t LastTick = xTaskGetTickCount();

    ADC_SAR_Seq_StartConvert();

    for (ever)
    {
        vTaskDelayUntil(&LastTick, MaxWait);
        knobs[0] = ADC_SAR_Seq_GetResult16(0);
        knobs[1] = ADC_SAR_Seq_GetResult16(1);
        knobs[2] = ADC_SAR_Seq_GetResult16(2);
    }
}

// CY_ISR_PROTO(adc_eoc_isr)
//{
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//
//     // New update from the sync_counter. This will increment the number of notifications.
//     vTaskNotifyGiveFromISR(KnobsTask, &xHigherPriorityTaskWoken);
//
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }
