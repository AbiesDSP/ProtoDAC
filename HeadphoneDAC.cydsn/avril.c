#include "avril.h"

static Avril self;

static int locate_interface(uint32_t address, uint32_t *virtual_base_address, AvrilInterface **found)
{
    int err = 0;

    int begin = 0;
    int end = self.n_interfaces - 1;

    int distance;
    int mid = 0;

    *virtual_base_address = 0;

    while (begin <= end)
    {
        mid = (begin + end) / 2;

        *virtual_base_address = self.addresses[mid];

        distance = address - *virtual_base_address;

        if (distance >= 0)
        {
            begin = mid + 1;
            *found = self.interfaces[mid];

            // The virtual address is within this address space.
            if (distance < self.interfaces[mid]->size)
            {
                break;
            }
        }
        else
        {
            end = mid - 1;
        }
    }

    // Check if address is valid.
    if (*found)
    {
        // Unmapped address
        if ((address - *virtual_base_address) >= (*found)->size)
        {
            *found = 0;
            mid = 0;
        }
    }

    return err;
}

void avril_init(void)
{
    self.n_interfaces = 0;
}

void avril_register(uint32_t virtual_address, AvrilInterface *interface, int size)
{
    self.addresses[self.n_interfaces] = virtual_address;
    self.interfaces[self.n_interfaces] = interface;
    self.interfaces[self.n_interfaces]->size = size;
    self.n_interfaces++;
}

int avril_execute(AvrilCommand *command)
{
    uint32_t virtual_base_address = 0;
    AvrilInterface *iface;

    int err = locate_interface(command->address & ~AVRIL_WRITE_EN, &virtual_base_address, &iface);
    if (err == 0)
    {
        command->address -= virtual_base_address;
        if (command->address & AVRIL_WRITE_EN)
        {
            command->address &= ~AVRIL_WRITE_EN;
            err = iface->write(iface, command);
        }
        else
        {
            err = iface->read(iface, command);
        }
    }

    return err;
}

int avril_write(uint32_t address, const uint32_t *src, int amount)
{
}
int avril_read(uint32_t address, uint32_t *dst, int amount)
{
}
