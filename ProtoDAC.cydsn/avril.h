#pragma once
#include <stdint.h>

// Max for a 64 byte packet.
#define AVRIL_MAX_MSG_SIZE 56
#define AVRIL_CMD_MAX_REGS 14
#define AVRIL_MAX_INTERFACES 64

#define AVRIL_WRITE_EN 0x80000000

/* Abies Virtual Register Interface */
typedef struct Avril Avril;
typedef struct AvrilCommand AvrilCommand;
typedef struct AvrilInterface AvrilInterface;

// amount is in bytes.
// Write data command.
typedef int (*AvrilWrite)(void *self, AvrilCommand *cmd);
typedef int (*AvrilRead)(void *self, AvrilCommand *cmd);

struct AvrilCommand
{
    uint32_t amount;
    uint32_t address;
    uint32_t regs[AVRIL_CMD_MAX_REGS];
};

struct AvrilInterface
{
    AvrilWrite write;
    AvrilRead read;
    int size;
};

struct Avril
{
    uint32_t addresses[AVRIL_MAX_INTERFACES];
    AvrilInterface *interfaces[AVRIL_MAX_INTERFACES];
    int n_interfaces;
};

void avril_init(void);
void avril_register(uint32_t virtual_address, AvrilInterface *iface, int size);

int avril_execute(AvrilCommand *command);
