#include "project_config.h"
#include "project.h"

#include "audio_tx.h"
#include "usb.h"
#include "sync.h"
#include "ear_saver.h"
#include "knobs.h"
#include "booter.h"
#include "avril.h"
#include "rtc.h"

#include "loggers.h"

static void prvHardwareSetup(void);
void SomeLogger(void *pvParameters);

int main(void)
{
#if !TEST_BUILD
    CyGlobalIntEnable;
#endif

    CyDelay(1);
    if (0 == (SwitchStatus_Read() & SW_BOOT))
    {
        Bootloadable_Load();
    }

    prvHardwareSetup();

    // Audio Transmit Tasks.
    TaskHandle_t AudioTxTask = NULL;
    xTaskCreate(AudioTxMonitor, "Audio Tx Monitor", LOG_MIN_STACK_SIZE, NULL, AUDIO_OUT_MONITOR_TASK_PRI, NULL);
    xTaskCreate(AudioTx, "Audio Tx", LOG_MIN_STACK_SIZE, usb_get_audio_out_ep_buf(), AUDIO_OUT_TASK_PRI, &AudioTxTask);
    xTaskCreate(AudioTxLogging, "Audio Tx Logging", LOG_MIN_STACK_SIZE, NULL, LOG_TASK_PRI, NULL);

    // USB Tasks
    usb_set_audio_output_task(AudioTxTask);
    xTaskCreate(USBConfigService, "USB Service Config", configMINIMAL_STACK_SIZE, NULL, USB_TASK_PRI, NULL);
    TaskHandle_t USBFbTask = NULL;
    xTaskCreate(USBServiceAudioFeedbackEp, "USB Feedback", LOG_MIN_STACK_SIZE, NULL, USB_TASK_PRI, &USBFbTask);

    // USB Serial Port
    xTaskCreate(USBSerialTx, "USB Serial Tx", configMINIMAL_STACK_SIZE, NULL, SERIAL_TASK_PRI, NULL);
    xTaskCreate(USBSerialRx, "USB Serial Rx", configMINIMAL_STACK_SIZE, NULL, SERIAL_TASK_PRI, NULL);

    // USB Synchronization Monitor. Updates USB with new updates on the sample rate.
    xTaskCreate(SyncMonitor, "Sync Monitor", LOG_MIN_STACK_SIZE, USBFbTask, SYNC_TASK_PRI, NULL);

    // Monitor for any error conditions that would cause unpleasant distortion, and automatically mute.
    xTaskCreate(EarSaver, "EarSaver", configMINIMAL_STACK_SIZE, NULL, EAR_SAVER_TASK_PRI, NULL);

    // Update the current state of the knobs.
    xTaskCreate(KnobsUpdate, "Knobs", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Bootload
    xTaskCreate(Booter, "Booter", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Real Time Clock. 1pps updates
    xTaskCreate(RTCUpdate, "RTC", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // xTaskCreate(SomeLogger, "Some Logger", 512, NULL, 1, NULL);

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
    rtc_init();
    audio_tx_init();
    usb_audio_init();
    usb_serial_init();
    sync_init();
    knobs_init();
    avril_init();
    avril_register(2048, &RTCIface, 4);
    avril_register(16384, &BooterIface, 4);

    // Configure serial logger
    logger_init(&main_log, &usb_serial_log_handler, NULL, NULL, NULL);

    main_log.level = GLOBAL_LOG_LEVEL;
}

void SomeLogger(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t log_interval = pdMS_TO_TICKS(500);

    for (ever)
    {
        vTaskDelay(log_interval);
        log_debug(&main_log, "Hello, World!\n");
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
