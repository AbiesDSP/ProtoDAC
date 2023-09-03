#pragma once
#include <stdint.h>

#define COMMAND_BUF_SIZE 56

// List of commands
#define COMMAND_ENTER_BOOTLOAD_ADDR 16384

typedef struct Command Command;
struct Command
{
    uint32_t amount;
    uint32_t address;
    uint8_t data[COMMAND_BUF_SIZE];
};