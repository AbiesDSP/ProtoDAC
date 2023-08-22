#include "project.h"
#include "usb.h"
#include "audio_out.h"
#include "audio_proc.h"

#include "loggers.h"
#include "serial.h"

#define ever (;;)

// Enable processing data before transmitting.
#define ENABLE_PROC 1

volatile int adc_update_flag = 0;
CY_ISR_PROTO(adc_isr);

volatile int sync_counter_flag = 0;
CY_ISR_PROTO(sync_counter_update_isr);

volatile int service_serial_tx_flag = 0;
CY_ISR_PROTO(serial_tx_isr);

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 1024
#define AUDIO_TX_N_TDS 8
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)
static uint8_t audio_tx_buf[AUDIO_TX_BUF_SIZE];

// Audio processing buffers.
static float audio_left[AUDIO_PROC_BUF_SIZE];
static float audio_right[AUDIO_PROC_BUF_SIZE];
static uint8_t usb_tx_buf[USB_MAX_BUF_SIZE];

// Serial library to send/receive over the uart
#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_TX_BUFFER_SIZE 1024
serial ser;
static uint8_t serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
static uint8_t serial_tx_buffer[SERIAL_TX_BUFFER_SIZE];

// Send log messages over serial port.
logger serial_log;
log_handler serial_handler;
int serial_handler_write(log_handler *self, const char *src, int amount);

int main(void)
{
    CyGlobalIntEnable;

    mute_sw_isr_StartEx(mute_isr);
    set_mute(AUDIO_ENABLED);

    // Configure USB
    usb_start(48000);

    // Configure usb audio output.
    audio_out_config audio_tx_config = {
        .transfer_size = AUDIO_TX_TRANSFER_SIZE,
        .n_tds = AUDIO_TX_N_TDS,
        .buffer = audio_tx_buf,
        .overflow_limit = 2,
    };
    audio_out_init(&audio_tx_config);

    // Configure serial
    serial_init(&ser, serial_rx_buffer, sizeof(serial_rx_buffer), serial_tx_buffer, sizeof(serial_tx_buffer));
    uart_tx_isr_StartEx(serial_tx_isr);

    // Configure serial logger
    log_handler_init_funcs(&serial_handler, serial_handler_write, NULL);
    logger_init(&serial_log, &serial_handler, NULL);
    
    serial_log.level = LOG_DEBUG;

    int n_samples = 0;
    float volume_gain = 1.0;
    float lpf_gain = 0.0;
    static float lpf_l_last = 0, lpf_r_last = 0;
    
    // Configure frame timer for usb synchronization.
    sync_counter_start(48000);
    sync_counter_isr_StartEx(sync_counter_update_isr);
    
    while (42)
    {   
        // Process audio here.
        if (usb_audio_out_update_flag)
        {
            usb_audio_out_update_flag = 0;

#if ENABLE_PROC
            n_samples = usb_audio_out_count / 6;

            // Unpack bytes into float arrays.
            unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_left, 0);
            unpack_usb_data_float(usb_audio_out_buf, n_samples, audio_right, 1);
            
            // Process left channel.
            lpf_exp(audio_left, proc_buf0, n_samples, lpf_gain, &lpf_l_last);
            volume(proc_buf0, audio_left, n_samples, volume_gain);
            // Process right channel.
            lpf_exp(audio_right, proc_buf0, n_samples, lpf_gain, &lpf_r_last);
            volume(proc_buf0, audio_right, n_samples, volume_gain);
            
            // Pack float array into bytes into usb_tx_buf
            pack_usb_data_float(usb_tx_buf, n_samples, audio_left, 0);
            pack_usb_data_float(usb_tx_buf, n_samples, audio_right, 1);

            // Send the processed data.
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

        // New feedback update.
        if (sync_counter_flag)
        {
            sync_counter_flag = 0;
            uint32_t sync_fb = sync_counter_read();
            
            // Update the feedback register
            usb_update_feedback(sync_fb);
            
            // Sample rate in hz.
            float sample_rate = (1000.0 * sync_fb) / 16384.0;
            // Tx buffer status.
            float buf_percent = 100.0 * ((float)audio_out_buffer_size / AUDIO_TX_BUF_SIZE);
            log_debug(&serial_log, "tx_buffer: %2.0f%%, sample_rate_fb: %.2f  \r", buf_percent, sample_rate);
        }

        // Transmit analog data to fpga.
        if (service_serial_tx_flag)
        {
            service_serial_tx_flag = 0;
            serial_service_tx_isr(&ser);
        }

        // New adc data.
        if (adc_update_flag)
        {
            adc_update_flag = 0;
            // analog_update_filter();
            // ADC_SAR_Seq_StartConvert();
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
}

int serial_handler_write(log_handler *self, const char *src, int amount)
{
    (void)self;
    serial_send(&ser, src, amount);
    return amount;
}
