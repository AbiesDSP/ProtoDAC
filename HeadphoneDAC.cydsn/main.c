#include "project.h"
#include "usb.h"
#include "audio_out.h"
#include "muter.h"

#include "logging.h"
#include "loggers.h"
#include "serial.h"
#include <string.h>
#include <stdio.h>

#define ever (;;)

uint8_t serial_rx_buffer[1024];
uint8_t serial_tx_buffer[1024];

volatile int adc_update_flag = 0;
CY_ISR_PROTO(adc_isr);

volatile int sync_counter_flag = 0;
CY_ISR_PROTO(sync_counter_update_isr);

CY_ISR_PROTO(serial_tx_isr);

serial ser;

int serial_handler_write(log_handler *self, const char *src, int amount)
{
    (void)self;
    serial_send(&ser, src, amount);
    return amount;
}

volatile int service_serial_tx_flag = 0;

#define AUDIO_TX_TRANSFER_SIZE 288
#define AUDIO_TX_N_TDS 32
static uint8_t audio_tx_buf[AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS];

logger serial_log;

int main(void)
{
    CyGlobalIntEnable;

    mute_sw_isr_StartEx(mute_isr);
    set_mute(AUDIO_ENABLED);

    usb_start(48000);

    // Configure usb audio output.
    audio_out_config audio_tx_config = {
        .transfer_size = AUDIO_TX_TRANSFER_SIZE,
        .n_tds = AUDIO_TX_N_TDS,
        .buffer = audio_tx_buf,
        .swap_endian = 1,
    };
    audio_out_init(&audio_tx_config);

    serial_init(&ser, serial_rx_buffer, sizeof(serial_rx_buffer), serial_tx_buffer, sizeof(serial_tx_buffer));
    tx_isr_StartEx(serial_tx_isr);

    log_handler serial_handler;
    log_handler_init_funcs(&serial_handler, serial_handler_write, NULL);

    log_formatter formatter;
    //sprintf_formatter_init(&formatter, "%s: ", "txsize: %d, srfb: %d        ");
    sprintf_formatter_init(&formatter, "%s: ", "%s");
    formatter.delim = '\n';

    logger_init(&serial_log, &serial_handler, &formatter);
    serial_log.level = LOG_WARN;

    FrameCount_Start();
    sync_counter_start();
    sync_counter_isr_StartEx(sync_counter_update_isr);

    for ever
    {
        // Process audio here.
        if (usb_audio_out_update_flag)
        {
            usb_audio_out_update_flag = 0;
            audio_out_transmit(usb_audio_out_buf, usb_audio_out_count);
        }
        
        // USB Handler
        if (USBFS_GetConfiguration())
        {
            usb_service();
        }

        // Update USB Measured FS buffer
        if (sync_counter_flag)
        {
            sync_counter_flag = 0;
            // Magic numbers.
            uint32_t new_usb_feedback = sync_counter_read() / 3.0;

            // Update the feedback register
            uint8_t int_status = CyEnterCriticalSection();
            sample_rate_feedback = new_usb_feedback;
            CyExitCriticalSection(int_status);

            int sample_rate = sample_rate_feedback / 16.384;
            log_info(&serial_log, audio_out_buffer_size, sample_rate);
        }
        
        // Transmit analog data to fpga.
        if (service_serial_tx_flag)
        {
            service_serial_tx_flag = 0;
//            serial_service_tx_isr(&ser);
        }

        // Update adc low-pass filter/oversampling
        if (adc_update_flag)
        {
            adc_update_flag = 0;
            //            analog_update_filter();
            //            ADC_SAR_Seq_StartConvert();
        }
    }
}

CY_ISR(adc_isr)
{
    adc_update_flag = 1;
}

CY_ISR(sync_counter_update_isr)
{
    sync_counter_flag = 1;
}

CY_ISR(serial_tx_isr)
{
    service_serial_tx_flag = 1;
    serial_service_tx_isr(&ser);
}
