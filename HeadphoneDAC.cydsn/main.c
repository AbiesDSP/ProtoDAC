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

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 288
#define AUDIO_TX_N_TDS 32
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)
static uint8_t audio_tx_buf[AUDIO_TX_BUF_SIZE];

// Audio processing buffers.
static float audio_left[AUDIO_PROC_BUF_SIZE];
static float audio_right[AUDIO_PROC_BUF_SIZE];
//
static uint8_t source_tx_buf[USB_MAX_BUF_SIZE];

// Serial library to send/receive over the uart
#define SERIAL_TX_BUFFER_SIZE 1024
static uint8_t serial_tx_buffer[SERIAL_TX_BUFFER_SIZE];
CY_ISR_PROTO(uart_tx_isr);

SerialTx serial_tx;

#define SERIAL_RX_BUFFER_SIZE 1024
static uint8_t serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
CY_ISR_PROTO(flush_isr);

SerialRx serial_rx;

#define SERIAL_MSG_BUF_SIZE 1024
static uint8_t serial_msg_buf[SERIAL_MSG_BUF_SIZE];

// Send log messages over the serial port.
Logger serial_log;
SerialLogHandler serial_log_handler;

int main(void)
{
    CyGlobalIntEnable;
    // Configure USB
    sync_counter_isr_StartEx(sync_counter_update_isr);
    sync_counter_start();
    
    // Start UART hardware
    UART_Start();

    usb_start(48000);

    mute_sw_isr_StartEx(mute_isr);
    set_mute(AUDIO_MUTED);

    /* Audio Transmit Configuration */
    AudioTxConfig audio_tx_config = {
        .transfer_size = AUDIO_TX_TRANSFER_SIZE,
        .n_tds = AUDIO_TX_N_TDS,
        .buffer = audio_tx_buf,
        .overflow_limit = 2,
    };
    audio_out_init(&audio_tx_config);

    /* Serial Transmit Configuration */
    SerialTxConfig serial_tx_config = {
        .buffer = serial_tx_buffer,
        .size = SERIAL_TX_BUFFER_SIZE,
        .dma_ch = DMATxUART_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE)),
        .max_dma_transfer_size = 4095,
        .uart_txdata_ptr = UART_TXDATA_PTR,
        .td_termout_en = DMATxUART__TD_TERMOUT_EN,
    };
    serial_tx_init(&serial_tx, &serial_tx_config);
    uart_tx_isr_StartEx(uart_tx_isr);

    /* Serial Receive Configuration */
    SerialRxConfig serial_rx_config = {
        .buffer = serial_rx_buffer,
        .size = SERIAL_RX_BUFFER_SIZE,
        .dma_ch = DMARxUART_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE)),
        .max_dma_transfer_size = 4095,
        .uart_rxdata_ptr = UART_RXDATA_PTR,
    };
    serial_rx_init(&serial_rx, &serial_rx_config);
    flush_isr_StartEx(flush_isr);

    // Configure serial logger
    serial_log_handler_init(&serial_log_handler, &serial_tx);
    logger_init(&serial_log, &serial_log_handler.handler, NULL);

    serial_log.level = LOG_INFO;

    int n_samples = 0;
    float volume_gain = 1.0;
    float lpf_gain = 0.0;
    static float lpf_l_last = 0, lpf_r_last = 0;

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
            pack_usb_data_float(source_tx_buf, n_samples, audio_left, 0);
            pack_usb_data_float(source_tx_buf, n_samples, audio_right, 1);

            // Send the processed data.
            audio_out_transmit(source_tx_buf, usb_audio_out_count);
#else
            // Send usb data directly
            audio_out_transmit(source_tx_buf, usb_audio_out_count);
#endif
        }
        
        // New Serial Command
        int rx_size = serial_rx_buffer_size(&serial_rx);
        if (rx_size)
        {
            serial_rx_receive(&serial_rx, serial_msg_buf, rx_size);
            serial_msg_buf[rx_size] = 0;// Make sure it has a delimeter...
            log_info(&serial_log, "%s", serial_msg_buf);
        }

        // USB Handler
        if (USBFS_GetConfiguration())
        {
            usb_service();
        }

        // New feedback update.
        if (sync_counter_flag)
        {
            static int dump_first = 1;
            sync_counter_flag = 0;

            // Update the feedback register
            uint32_t sfb = sync_counter_read();
            // First measurement has garbage...
            if (dump_first)
            {
                dump_first = 0;
                log_debug(&serial_log, "Garbage Sync Feedback Counter: %d\n", sfb);
            }
            else
            {
                int32_t rolling_average = usb_update_feedback(sfb);
                if (rolling_average)
                {
                    // Sample rate in hz.
                    float sample_rate = (1000.0 * rolling_average) / 16384.0;
                    // Tx buffer status.
                    float buf_percent = 100.0 * ((float)audio_out_buffer_size / AUDIO_TX_BUF_SIZE);
                    log_debug(&serial_log, "Tx Buffer Size: %.0f%%, Relative Sample Rate: %f  \r", buf_percent, sample_rate);
                }
            }
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

CY_ISR(uart_tx_isr)
{
    serial_tx_isr(&serial_tx);
}

CY_ISR(flush_isr)
{
    serial_rx_isr(&serial_rx, Flush_TransferSize());
}
