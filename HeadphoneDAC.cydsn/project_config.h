#pragma once

// Enable processing data before transmitting.
#define ENABLE_PROC 1

// Audio transmit buffer
#define AUDIO_TX_TRANSFER_SIZE 288
#define AUDIO_TX_N_TDS 32
#define AUDIO_TX_BUF_SIZE (AUDIO_TX_TRANSFER_SIZE * AUDIO_TX_N_TDS)

// Serial port
#define SERIAL_TX_BUFFER_SIZE 1024
#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_MSG_BUF_SIZE 1024

#define GLOBAL_LOG_LEVEL LOG_DEBUG

