#include "project.h"
#include "usb.h"
#include "audio_out.h"
#include "audio_proc.h"
#include "muter.h"

#include "logging.h"
#include "loggers.h"
#include "serial.h"
#include <string.h>
#include <stdio.h>

#define ever (;;)

#define ENABLE_PROC 1

volatile int adc_update_flag = 0;
CY_ISR_PROTO(adc_isr);

volatile int sync_counter_flag = 0;
CY_ISR_PROTO(sync_counter_update_isr);

volatile int service_serial_tx_flag = 0;
CY_ISR_PROTO(serial_tx_isr);

// Serial library to send/receive over the uart
#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_TX_BUFFER_SIZE 1024
serial ser;
uint8_t serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
uint8_t serial_tx_buffer[SERIAL_TX_BUFFER_SIZE];

// Serial log handler
int serial_handler_write(log_handler *self, const char *src, int amount);
log_handler serial_handler;

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 288
#define AUDIO_TX_N_TDS 32
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)
static uint8_t audio_tx_buf[AUDIO_TX_BUF_SIZE];

static float audio_left[AUDIO_PROC_BUF_SIZE];
static float audio_right[AUDIO_PROC_BUF_SIZE];

static int lbuf[AUDIO_PROC_BUF_SIZE];
static int rbuf[AUDIO_PROC_BUF_SIZE];

static uint8_t usb_tx_buf[USB_MAX_BUF_SIZE];
//
logger serial_log;
logger serial_err_log;

#define SAMPLE_RATE 48000

int main(void)
{
    CyGlobalIntEnable;

    mute_sw_isr_StartEx(mute_isr);
    set_mute(AUDIO_ENABLED);

    // Configure USB
    usb_start(SAMPLE_RATE);

    // Configure usb audio output.
    audio_out_config audio_tx_config = {
        .transfer_size = AUDIO_TX_TRANSFER_SIZE,
        .n_tds = AUDIO_TX_N_TDS,
        .buffer = audio_tx_buf,
        .overflow_limit = 4,
    };
    audio_out_init(&audio_tx_config);

    // Configure serial
    serial_init(&ser, serial_rx_buffer, sizeof(serial_rx_buffer), serial_tx_buffer, sizeof(serial_tx_buffer));
    tx_isr_StartEx(serial_tx_isr);

    // Configure serial logger
    log_handler_init_funcs(&serial_handler, serial_handler_write, NULL);
    log_formatter formatter;
    sprintf_formatter_init(&formatter, "%s: ", "txsize: %d%%, srfb: %d        ");
    // sprintf_formatter_init(&formatter, "%s: ", "%s");
    formatter.delim = '\r';

    logger_init(&serial_log, &serial_handler, &formatter);
    serial_log.level = LOG_DEBUG;

    // Configure frame timer for usb synchronization.
    FrameCount_Start();
    sync_counter_start();
    sync_counter_isr_StartEx(sync_counter_update_isr);

    static float lpf_l_last = 0, lpf_r_last = 0;
    while (42)
    {
        // Process audio here.
        if (usb_audio_out_update_flag)
        {
            usb_audio_out_update_flag = 0;

#if ENABLE_PROC
            float gain = 1.0;
            float lpf_a = 0.000001;

            int n_samples = usb_audio_out_count / 6;

            unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_left, 0);
            unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_right, 1);

            lpf_exp(audio_left, proc_buf0, n_samples, lpf_a, &lpf_l_last);
            volume(proc_buf0, proc_buf1, n_samples, gain);

            pack_usb_data_float(usb_tx_buf, n_samples, proc_buf1, 0);

            lpf_exp(audio_right, proc_buf0, n_samples, lpf_a, &lpf_r_last);
            volume(proc_buf0, proc_buf1, n_samples, gain);

            pack_usb_data_float(usb_tx_buf, n_samples, proc_buf1, 1);

            audio_out_transmit(usb_tx_buf, usb_audio_out_count);
#else
            // Send usb data directly
            audio_out_transmit(usb_audio_out_buf, usb_audio_out_count);
#endif
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
            uint32_t new_usb_feedback = 0.5 + (float)sync_counter_read() / 3.0;

            // Update the feedback register
            uint8_t int_status = CyEnterCriticalSection();
            sample_rate_feedback = new_usb_feedback;
            CyExitCriticalSection(int_status);

            int sample_rate = sample_rate_feedback / 16.384;
            int buf_percent = 100.0 * ((float)audio_out_buffer_size / AUDIO_TX_BUF_SIZE);
            log_info(&serial_log, buf_percent, sample_rate);
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

int serial_handler_write(log_handler *self, const char *src, int amount)
{
    (void)self;
    serial_send(&ser, src, amount);
    return amount;
}
