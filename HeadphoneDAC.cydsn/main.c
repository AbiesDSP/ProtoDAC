#include "project.h"
#include "usb.h"
#include "audio_out.h"

uint16_t audio_out_buf_size_buf[2048];
uint16_t audio_out_buf_size_index = 0;
#define AO_BUF_SIZE_INDEX_MASK 0x7FF

volatile int update_100ms_flag = 0;
CY_ISR_PROTO(update_100ms_isr);

volatile int adc_update_flag = 0;
CY_ISR_PROTO(adc_isr);

volatile int sync_counter_flag = 0;
CY_ISR_PROTO(sync_counter_update_isr);

int main(void)
{
    CyGlobalIntEnable;

    // Configure usb audio output.
    audio_out_start();

    usb_start(48000);

//    // Configure comm channels
//    comm_tx_init();
//    comm_rx_init();

//    // Start the UART hardware
//    tx_isr_StartEx(comm_tx_isr);
//    flush_isr_StartEx(comm_rx_isr);
//    Flush_Init(0xFFFF, 0xFFFF);
//    UART_Start();

//
//    // Analog data
//    adc_eoc_isr_StartEx(adc_isr);

//    // Set up eeprom
//    eeprom_init();
//    usb_fs_measured.sram_buf = (uint8_t *)usb_fs_measured_buf;
//    audio_out_buf_size.sram_buf = (uint8_t *)audio_out_buf_size_buf;
//
//    for (int i = 0; i < (USB_FS_MEASURED_SIZE / 4); i++)
//    {
//        usb_fs_measured_buf[i] = 0;
//    }
//    avril_init(avril_nodes, N_INTERFACES);
//
//    // Timer_1_Start();
//    // timer_1_isr_StartEx(update_100ms_isr);
//
//    analog_start();

    FrameCount_Start();
    sync_counter_start();
    sync_counter_isr_StartEx(sync_counter_update_isr);

    int buf_size_update_interval = 0;

    for (;;)
    {
        // USB Handler
        if (USBFS_GetConfiguration())
        {
            usb_service();
        }

        // Transmit analog data to fpga.
        if (update_100ms_flag)
        {
            update_100ms_flag = 0;

//            analog_read_all(knob_data.knobs);
//            while (spi_status != SPI_DONE)
//                ;
//            spi_transaction((uint8_t *)&knob_data, sizeof(knob_data), NULL, 0);
        }

        // Update adc low-pass filter/oversampling
        if (adc_update_flag)
        {
            adc_update_flag = 0;
//            analog_update_filter();
//            ADC_SAR_Seq_StartConvert();
        }

        if (audio_out_update_flag)
        {
            audio_out_update_flag = 0;

            if (buf_size_update_interval++ == 0)
            {
                buf_size_update_interval = 0;
                audio_out_buf_size_buf[audio_out_buf_size_index++ & AO_BUF_SIZE_INDEX_MASK] = audio_out_buffer_size;
            }
        }

        // Update USB Measured FS buffer
        if (sync_counter_flag)
        {
            update_100ms_flag = 1;
            sync_counter_flag = 0;
            // Magic numbers.
            uint32_t new_usb_feedback = sync_counter_read() / 3.0;

//            const int log_int = 32;

//            usb_fs_measured_buf[usb_fs_measured_index & USB_FS_MEASURED_ADDR_MASK] = new_usb_feedback;
//            usb_fs_measured_index++;
//            if (usb_fs_measured_index == log_int)
//            {
//                usb_fs_measured_index = 0;
//            }

            // Update the feedback register
            uint8_t int_status = CyEnterCriticalSection();
            sample_rate_feedback = new_usb_feedback;
            CyExitCriticalSection(int_status);
        }
    }
}

//void handle_commands(void)
//{
//    size_t rx_size = comm_rx_buffer_size();
//
//    if (rx_size)
//    {
//        if (comm_rx_status)
//        {
//            comm_rx_status--;
//        }
//
//        // Check if there is a delimeted packet.
//        size_t next_packet_size = comm_rx_find(0);
//
//        // Expected message size was found.
//        if (next_packet_size < rx_size)
//        {
//            size_t decoded = comm_rx_decode((uint8_t *)&msg, next_packet_size);
//            (void)decoded; // Check if decoded == command.command.length + AVRIL_HEADER_SIZE
//            int write_cmd = (msg.command.address & AVRIL_WRITE_CMD);
//            avril_exec(msg.command, msg.data);
//            // Read command requires a response
//            if (write_cmd == 0)
//            {
//                comm_tx_encode((uint8_t *)&msg, msg.command.length + AVRIL_HEADER_SIZE);
//            }
//        }
//    }
//}

CY_ISR(update_100ms_isr)
{
    update_100ms_flag = 1;
}

CY_ISR(adc_isr)
{
    adc_update_flag = 1;
}

CY_ISR(sync_counter_update_isr)
{
    sync_counter_flag = 1;
}
