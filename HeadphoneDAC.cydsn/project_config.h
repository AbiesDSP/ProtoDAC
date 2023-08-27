#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portable.h"

#include "log_level.h"

// Enable processing data before transmitting.
#define ENABLE_PROC 1

// Initial sample rate
#define AUDIO_SAMPLE_RATE 48000

// More than enough for 96kHz
#define USB_MAX_BUF_SIZE 1024

// USB Synchronization
#define SYNC_WINDOW_SIZE 128
#define SYNC_N_WINDOWS 4
#define SYNC_SHIFT 3 // log2(SYNC_N_WINDOWS * SYNC_WINDOW_SIZE / 64)

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 512
#define AUDIO_TX_N_TDS 16
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)
#define AUDIO_TX_ACTIVE_LIMIT ((AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)>>1)
#define AUDIO_TX_OVERFLOW_LIMIT (AUDIO_TX_BUF_SIZE - AUDIO_TX_TRANSFER_SIZE * 4)
#define AUDIO_TX_UNDERFLOW_LIMIT (AUDIO_TX_TRANSFER_SIZE * 4)

// Serial port
#define SERIAL_TX_BUFFER_SIZE 2048
#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_MSG_BUF_SIZE 1024

#define SERIAL_TX_MAX_TDS 4
#define SERIAL_TX_MAX_TRANSFER_SIZE 4095

#define GLOBAL_LOG_LEVEL LOG_DEBUG
#define LOG_MESSAGE_BUF_SIZE 1024

#define ever \
    ;        \
    ;
