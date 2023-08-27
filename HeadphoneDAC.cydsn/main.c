#include "project_config.h"
#include "project.h"

// #include "audio_proc.h"
// #include "muter.h"

#include "audio_tx.h"
#include "usb.h"
#include "sync.h"
#include "serial.h"

#include "loggers.h"

// volatile int adc_update_flag = 0;
// CY_ISR_PROTO(adc_isr);

// // Audio processing buffers.
// static float audio_left[AUDIO_PROC_BUF_SIZE];
// static float audio_right[AUDIO_PROC_BUF_SIZE];
// static uint8_t source_tx_buf[USB_MAX_BUF_SIZE];

// static uint8_t serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
// CY_ISR_PROTO(flush_isr);

// SerialRx serial_rx;

static void prvHardwareSetup(void);

void SomeLogger(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t log_interval = pdMS_TO_TICKS(1000);

    for (ever)
    {
        vTaskDelay(log_interval);
        //log_debug(&serial_log, "Hello, World!\n");
    }
}

int main(void)
{
#if !TEST_BUILD
    CyGlobalIntEnable;
#endif

    prvHardwareSetup();
    
    // Audio Transmit Tasks.
    TaskHandle_t xAudioOutTask = NULL;
    xTaskCreate(AudioTransmit, "Audio Transmit", 512, (void*)usb_audio_out_buf, 5, &xAudioOutTask);
    xTaskCreate(AudioTxMonitor, "Audio Tx Monitor", 512, NULL, 4, NULL);
    
    // USB Tasks
    usb_set_audio_output_task(xAudioOutTask);
    xTaskCreate(USBServiceAudioFeedbackEp, "USB Feedback", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    xTaskCreate(USBConfigService, "USB Service Config", configMINIMAL_STACK_SIZE, NULL, 3, NULL);

    // USB Synchronization Monitor
    xTaskCreate(SyncMonitor, "Sync Monitor", 512, NULL, 1, NULL);
    
    // Serial Transmit Port
    xTaskCreate(SerialSender, "Serial Sender", 512, NULL, 2, NULL);
    // Logger to test the serial logging.
    //xTaskCreate(SomeLogger, "Some Logger", 1024, NULL, 1, NULL);
    xTaskCreate(AudioTxLogging, "Audio Tx Logging", 512, NULL, 1, NULL);
    vTaskStartScheduler();

    // mute_sw_isr_StartEx(mute_isr);
    // set_mute(AUDIO_MUTED);

    // uart_tx_isr_StartEx(uart_tx_isr);

    // /* Serial Receive Configuration */
    // SerialRxConfig serial_rx_config = {
    //     .buffer = serial_rx_buffer,
    //     .size = SERIAL_RX_BUFFER_SIZE,
    //     .dma_ch = DMARxUART_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE)),
    //     .max_dma_transfer_size = 4095,
    //     .uart_rxdata_ptr = UART_RXDATA_PTR,
    // };
    // serial_rx_init(&serial_rx, &serial_rx_config);
    // flush_isr_StartEx(flush_isr);

    // serial_log.level = GLOBAL_LOG_LEVEL;

    // int n_samples = 0;
    // float volume_gain = 1.0;
    // float lpf_gain = 0.0;
    // static float lpf_l_last = 0, lpf_r_last = 0;

    for (ever)
    {
        //         // Process audio here.
        //         if (usb_audio_out_update_flag)
        //         {
        //             usb_audio_out_update_flag = 0;

        // #if ENABLE_PROC
        //             n_samples = usb_audio_out_count / 6;

        //             // Unpack bytes into float arrays.
        //             unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_left, 0);
        //             unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_right, 1);

        //             // Process left channel.
        //             lpf_exp(audio_left, proc_buf0, n_samples, lpf_gain, &lpf_l_last);
        //             volume(proc_buf0, audio_left, n_samples, volume_gain);
        //             // Process right channel.
        //             lpf_exp(audio_right, proc_buf0, n_samples, lpf_gain, &lpf_r_last);
        //             volume(proc_buf0, audio_right, n_samples, volume_gain);

        //             // Pack float array into bytes into usb_tx_buf
        //             pack_usb_data_float(source_tx_buf, n_samples, audio_left, 0);
        //             pack_usb_data_float(source_tx_buf, n_samples, audio_right, 1);

        //             // Send the processed data.
        //             audio_out_transmit(source_tx_buf, usb_audio_out_count);
        // #else
        //             // Send usb data directly
        //             audio_out_transmit(source_tx_buf, usb_audio_out_count);
        // #endif
        //         }

        //         // New Serial Command
        //         int rx_size = serial_rx_buffer_size(&serial_rx);
        //         if (rx_size)
        //         {
        //             serial_rx_receive(&serial_rx, serial_msg_buf, rx_size);
        //             serial_msg_buf[rx_size] = 0; // Make sure it has a delimeter...
        //             log_info(&serial_log, "%s", serial_msg_buf);
        //         }

        //         // USB Handler
        //         if (USBFS_GetConfiguration())
        //         {
        //             usb_service();
        //         }

        //         // New adc data.
        //         if (adc_update_flag)
        //         {
        //             adc_update_flag = 0;
        //             // analog_update_filter();
        //             // ADC_SAR_Seq_StartConvert();
        //         }
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

    // Configure serial logger
    logger_init(&serial_log, &serial_log_handler, NULL, NULL, NULL);
    serial_log.level = GLOBAL_LOG_LEVEL;

    /* Start-up the peripherals. */
    audio_out_init();
    usb_init();
    sync_init();
    serial_tx_init();
}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
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

// CY_ISR(adc_isr)
// {
//     adc_update_flag = 1;
// }
