#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portable.h"
#include "log_level.h"

// Enable processing data before transmitting.
#define ENABLE_PROC 1

// Switches
#define SW_BOOT 0x8
#define SW_SOMESWITCH 0x4
#define SW_MUTE 0x2

// Stack sizes
#define LOG_MIN_STACK_SIZE 512

// Task priorities
#define AUDIO_OUT_TASK_PRI 5
#define AUDIO_OUT_MONITOR_TASK_PRI 4
#define USB_TASK_PRI 3
#define SYNC_TASK_PRI 2
#define SERIAL_TASK_PRI 2
#define EAR_SAVER_TASK_PRI 2
#define LOG_TASK_PRI 1

// Initial sample rate
#define AUDIO_SAMPLE_RATE 48000

// Enough for 96kHz
#define USB_AUDIO_EP_BUF_SIZE 588
#define USB_CONFIG_SERVICE_MAX_WAIT 100
#define USB_FEEDBACK_MAX_WAIT 60
#define USB_SERIAL_EP_BUF_SIZE 64

// USB Interface Configuration
#define USBFS_DEVICE 0
#define USB_ALT_INVALID 0xFF

#define USB_AUDIO_CTL_IFACE 0
#define USB_AUDIO_CTL_EP 5

#define USB_AUDIO_OUT_IFACE 1
#define USB_AUDIO_OUT_EP 1
#define USB_AUDIO_OUT_N_ALT 2
#define USB_AUDIO_OUT_ALT_ZEROBW 0
#define USB_AUDIO_OUT_ALT_STREAM 1

#define USB_AUDIO_FB_EP 3
// #define USB_AUDIO_IN_EP 2

#define USB_AUDIO_HID_IFACE 2
#define USB_AUDIO_HID_EP 4

//
// USB Serial Port
//
#define USB_SERIAL_CTL_IFACE 3
#define SERIAL_CTL_EP 6

#define USB_SERIAL_DATA_IFACE 4
#define USB_SERIAL_TX_EP 7
#define USB_SERIAL_RX_EP 8

#define USB_SERIAL_TX_BUF_SIZE 1024

// USB Synchronization
#define SYNC_WINDOW_SIZE 128
#define SYNC_N_WINDOWS 8
#define SYNC_SHIFT 4 // log2(SYNC_N_WINDOWS * SYNC_WINDOW_SIZE / 64)
#define SYNC_START_DELAY 200
#define SYNC_MAX_WAIT 60

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 588
#define AUDIO_TX_N_TDS 16
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)
// Turn on audio when it's half full
#define AUDIO_TX_ACTIVE_LIMIT ((AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS) >> 1)

#define AUDIO_TX_OVERFLOW_BLOCKS 4
#define AUDIO_TX_OVERFLOW_LIMIT (AUDIO_TX_BUF_SIZE - AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_OVERFLOW_BLOCKS)
#define AUDIO_TX_UNDERFLOW_LIMIT (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_OVERFLOW_BLOCKS)

#define AUDIO_TX_TX_MAX_WAIT 1
#define AUDIO_TX_MONITOR_MAX_WAIT 1
#define AUDIO_TX_LOGGING_INTERVAL 2000
#define AUDIO_TX_DELTA_QUEUE_SIZE 32
#define AUDIO_TX_MAX_BS_TDS ((AUDIO_TX_BUF_SIZE + DMA_MAX_TRANSFER_SIZE - 1) / DMA_MAX_TRANSFER_SIZE) // Sufficient number of tds for entire memory.

// Ear Saver Config
#define EAR_SAVER_STARTUP_DELAY 100
#define EAR_SAVER_RESET_INTERVAL 2

// Logging
#define GLOBAL_LOG_LEVEL LOG_DEBUG
#define LOG_MESSAGE_BUF_SIZE 1024

// Bootloader
#define BOOTER_HOLD_WAIT 3000
#define BOOTER_REFRESH_WAIT 500

//
#define DMA_MAX_TRANSFER_SIZE 4095

#define ever \
    ;        \
    ;
