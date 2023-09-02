#include "project_config.h"
#include "project.h"

#include "audio_tx.h"
#include "usb.h"
#include "sync.h"
#include "ear_saver.h"

#include "serial.h"

#include "loggers.h"

static void prvHardwareSetup(void);
void SomeLogger(void *pvParameters);
void Booter(void *pvParameters);

int main(void)
{
#if !TEST_BUILD
    CyGlobalIntEnable;
#endif

    prvHardwareSetup();

    // Audio Transmit Tasks.
    TaskHandle_t xAudioTxTask = NULL;
    xTaskCreate(AudioTxMonitor, "Audio Tx Monitor", LOG_MIN_STACK_SIZE, NULL, AUDIO_TX_MONITOR_TASK_PRI, NULL);
    xTaskCreate(AudioTx, "Audio Tx", 1024, (void *)usb_audio_out_buf, AUDIO_TX_TASK_PRI, &xAudioTxTask);
    xTaskCreate(AudioTxLogging, "Audio Tx Logging", LOG_MIN_STACK_SIZE, NULL, LOG_TASK_PRI, NULL);

    // USB Tasks
    usb_set_audio_output_task(xAudioTxTask);
    xTaskCreate(USBConfigService, "USB Service Config", configMINIMAL_STACK_SIZE, NULL, USB_TASK_PRI, NULL);
    xTaskCreate(USBServiceAudioFeedbackEp, "USB Feedback", configMINIMAL_STACK_SIZE, NULL, USB_TASK_PRI, NULL);

    // USB Synchronization Monitor
    xTaskCreate(SyncMonitor, "Sync Monitor", LOG_MIN_STACK_SIZE, NULL, SYNC_TASK_PRI, NULL);

    // Serial Port
    xTaskCreate(SerialSender, "Serial Sender", configMINIMAL_STACK_SIZE, NULL, SERIAL_TASK_PRI, NULL);
    xTaskCreate(SerialReceiver, "Serial Receiver", configMINIMAL_STACK_SIZE, NULL, SERIAL_TASK_PRI, NULL);

    // Monitor for any error conditions that would cause unpleasant distortion, and automatically mute.
    xTaskCreate(EarSaver, "EarSaver", configMINIMAL_STACK_SIZE, NULL, EAR_SAVER_TASK_PRI, NULL);

    // Bootload
    xTaskCreate(Booter, "Booter", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    //    xTaskCreate(SomeLogger, "Some Logger", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    for (ever)
    {
    }
}

void prvHardwareSetup(void)
{
    /* Port layer functions that need to be copied into the vector table. */
    extern void xPortPendSVHandler(void);
    extern void xPortSysTickHandler(void);
    extern void vPortSVCHandler(void);
    extern cyisraddress CyRamVectors[];

    /* Install the OS Interrupt Handlers. */
    CyRamVectors[11] = (cyisraddress)vPortSVCHandler;
    CyRamVectors[14] = (cyisraddress)xPortPendSVHandler;
    CyRamVectors[15] = (cyisraddress)xPortSysTickHandler;

    /* Start-up the peripherals. */
    audio_tx_init();
    usb_init();
    sync_init();
    serial_tx_init();
    serial_rx_init();

    // Configure serial logger
    logger_init(&serial_log, &serial_log_handler, NULL, NULL, NULL);
    serial_log.level = GLOBAL_LOG_LEVEL;
}

void Booter(void *pvParameters)
{
    (void)pvParameters;
    
    // Boot button must be held this long to trigger bootloading.
    const TickType_t xHoldDelay = pdMS_TO_TICKS(BOOTER_HOLD_WAIT);
    
    // Check the state of the button this often.
    const TickType_t xRefreshDelay = pdMS_TO_TICKS(BOOTER_REFRESH_WAIT);
    
    // If the boot switch is held on startup, start the bootloader.
    if ((SwitchStatus_Read() & SW_BOOT) == 0)
    {
        Bootloadable_Load();
    }
    
    TickType_t xLastTime = xTaskGetTickCount();
    
    for (ever)
    {
        vTaskDelayUntil(&xLastTime, xRefreshDelay);
        // Boot switch is held down.
        if (!(SwitchStatus_Read() & SW_BOOT))
        {
            // Check if it's held down for xHoldDelay.
            vTaskDelayUntil(&xLastTime, xHoldDelay);
            if (!(SwitchStatus_Read() & SW_BOOT))
            {
                // Start the bootloader.
                Bootloadable_Load();
            }
        }
    }
}

void SomeLogger(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t log_interval = pdMS_TO_TICKS(1000);

    for (ever)
    {
        vTaskDelay(log_interval);
        log_debug(&serial_log, "Hello, World!\n");
    }
}

/*---------------------------------------------------------------------------*/
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;
    (void)pcTaskName;
    /* The stack space has been exceeded for a task, considering allocating more. */
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
/*---------------------------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* The heap space has been exceeded. */
    taskDISABLE_INTERRUPTS();
    for (;;)
        ;
}
/*---------------------------------------------------------------------------*/
