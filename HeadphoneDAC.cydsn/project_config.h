#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portable.h"

#include "log_level.h"

// Enable processing data before transmitting.
#define ENABLE_PROC 1

// Stack sizes
#define LOG_MIN_STACK_SIZE 512

// Task priorities
#define AUDIO_TX_TASK_PRI 5
#define AUDIO_TX_MONITOR_TASK_PRI 4
#define USB_TASK_PRI 3
#define SYNC_TASK_PRI 2
#define SERIAL_TASK_PRI 2
#define EAR_SAVER_TASK_PRI 2
#define LOG_TASK_PRI 1

// Initial sample rate
#define AUDIO_SAMPLE_RATE 48000

// More than enough for 96kHz
#define USB_MAX_BUF_SIZE 1024
#define USB_CONFIG_SERVICE_MAX_WAIT 10
#define USB_FEEDBACK_MAX_WAIT 60

#define USBFS_AUDIO_DEVICE 0
#define AUDIO_INTERFACE 1
#define AUDIO_OUT_EP 1
#define AUDIO_IN_EP 2
#define AUDIO_FB_EP 3

// USB Synchronization
#define SYNC_WINDOW_SIZE 128
#define SYNC_N_WINDOWS 8
#define SYNC_SHIFT 4 // log2(SYNC_N_WINDOWS * SYNC_WINDOW_SIZE / 64)
#define SYNC_START_DELAY 200
#define SYNC_MAX_WAIT 60

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 1152
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

// Serial port
#define SERIAL_TX_BUFFER_SIZE 2048
#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_MSG_BUF_SIZE 1024

#define SERIAL_TX_MAX_TDS 4
#define SERIAL_TX_MAX_TRANSFER_SIZE 4095

#define SERIAL_RX_MAX_TRANSFER_SIZE 4095
#define SERIAL_RX_MAX_TDS 4

// Logging
#define GLOBAL_LOG_LEVEL LOG_INFO
#define LOG_MESSAGE_BUF_SIZE 1024

//
#define DMA_MAX_TRANSFER_SIZE 4095

#define ever \
    ;        \
    ;
