#include "project_config.h"
#include "sync.h"
#include "sync_counter.h"
#include "sync_counter_isr.h"

#include "usb.h"
#include "audio_tx.h"

#include "knobs.h"

#include "loggers.h"

CY_ISR_PROTO(sync_counter_update_isr);

static TaskHandle_t SyncMonitorTask = NULL;
static TaskHandle_t AudioFbTask = NULL;

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

int update_rolling_average(uint32_t feedback, uint32_t *average)
{
    int update = 0;
    running_sum += (feedback - rolling_average_buf[feedback_iter]);
    rolling_average_buf[feedback_iter++] = feedback;

    if (feedback_iter == SYNC_N_WINDOWS)
    {
        feedback_iter = 0;
        update = 1;
    }
    *average = running_sum >> SYNC_SHIFT;
    return update;
}

void SyncMonitor(void *_AudioFbTask)
{
    AudioFbTask = _AudioFbTask;
    SyncMonitorTask = xTaskGetCurrentTaskHandle();

    // isr arrives every 128ms. So if it times out, usb may have stopped.
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(SYNC_MAX_WAIT);

    // Delay on startup to dump the first, inaccurate counter value
    const TickType_t xStartDelay = pdMS_TO_TICKS(SYNC_START_DELAY);
    vTaskDelay(xStartDelay);

    // Don't update until the rolling average buffer is valid.
    int locked = 0;

    // Clear the value in the counter.
    (void)sync_counter_read();

    for (ever)
    {
        // New update from sync_counter
        if (ulTaskNotifyTake(pdFALSE, xMaxBlockTime))
        {
            /* The transmission ended as expected. */
            // Update the feedback register
            uint32_t sfb = sync_counter_read();

            // Check if it's out of range?
            uint32_t rolling_average = 0;

            // Filter the feeback with a rolling average.
            if (update_rolling_average(sfb, &rolling_average))
            {
                int sample_rate = 0.5 + (float)rolling_average / 16.384;
                // Tx buffer status.
                // int buf_percent = (100 * audio_tx_size() / AUDIO_TX_BUF_SIZE);
                log_debug(&main_log, "SR: %d, fb: %d\n", sample_rate, (int)sfb);
                locked = 1;
            }
            // Update usb once buffer is full.
            if (locked)
            {
                xTaskNotify(AudioFbTask, rolling_average, eSetValueWithOverwrite);
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

    // New update from the sync_counter. This will increment the number of notifications.
    vTaskNotifyGiveFromISR(SyncMonitorTask, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
