#include "sync.h"
#include "project_config.h"

#include "sync_counter.h"
#include "sync_counter_isr.h"

#include "usb.h"
#include "audio_tx.h"

#include "loggers.h"

CY_ISR_PROTO(sync_counter_update_isr);

static TaskHandle_t xSyncMonitor = NULL;

void sync_init(void)
{
    // Configure USB
    sync_counter_isr_StartEx(sync_counter_update_isr);
    sync_counter_start();
}

// Running average over 8 128ms captures. 1.024 seconds.
static uint32_t rolling_average_buf[SYNC_N_WINDOWS];
static uint32_t running_sum = 0;
static int feedback_iter = 0;

int locked = 0;
int update_rolling_average(uint32_t feedback, int32_t *average)
{   
    int ready = 0;
    running_sum += (feedback - rolling_average_buf[feedback_iter]);
    rolling_average_buf[feedback_iter++] = feedback;

    if (feedback_iter == SYNC_N_WINDOWS)
    {
        feedback_iter = 0;
        locked = 1;
    }
    *average = running_sum >> SYNC_SHIFT;
    return locked;
}

void SyncMonitor(void *pvParameters)
{
    (void) pvParameters;
    
    float sample_rate = 0;
    
    xSyncMonitor = xTaskGetCurrentTaskHandle();

    // isr arrives every 128ms. So if it times out, usb may have stopped.
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200);
    uint32_t notifications = 0;

    int dump_first = 1;
    
    const TickType_t xStartDelay = pdMS_TO_TICKS(2000);
    int interval = 64;
    int i = 0;
    for (ever)
    {
        notifications = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);

        // New update from sync_counter
        if (notifications)
        {
            /* The transmission ended as expected. */
            // Update the feedback register
            uint32_t sfb = sync_counter_read();
            // First measurement has garbage...
            // Check if it's out of range?
            if (dump_first)
            {
                dump_first = 0;
                // log_debug(&serial_log, "Garbage Sync Feedback Counter: %d\n", sfb);
            }
            else
            {
                int32_t rolling_average;
                if (update_rolling_average(sfb, &rolling_average))
                {
                    usb_update_feedback(rolling_average);
                }
                i++;
                if (i == interval)
                {
                    i = 0;
    //                // Sample rate in hz.
                    sample_rate = (1000.0 * rolling_average) / 16384.0;
                    int sample_rate_i = sample_rate;
                    int sample_rate_frac = (float)(sample_rate-(float)sample_rate_i)*1000000.0;
                    // Tx buffer status.
                    //int buf_percent = (100 * audio_tx_buffer_size() / AUDIO_TX_BUF_SIZE);
                    log_debug(&serial_log, "SR %: %d_%d    \n", sample_rate_i, sample_rate_frac);
                }
            }
        }
        else
        {
            /* The call to ulTaskNotifyTake() timed out. */
            // log_debug(&serial_log, "Sync Feedback Timed Out!\n");
        }
    }
}

CY_ISR(sync_counter_update_isr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Notify the task that the transmission is complete. */
    vTaskNotifyGiveFromISR(xSyncMonitor, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
