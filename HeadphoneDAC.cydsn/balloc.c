#include "balloc.h"
#include <stdint.h>

static uint8_t mem_pool[BALLOC_BLOCK_SIZE * BALLOC_N_BLOCKS];
static void *allocated[BALLOC_N_BLOCKS];

static void balloc_init(void)
{
    static int is_initialized = 0;
    if (!is_initialized)
    {
        for (int i = 0; i < sizeof(mem_pool); i++)
        {
            mem_pool[i] = 0xFF;
        }
        is_initialized = 1;
    }
};

void *balloc(size_t size)
{
    balloc_init();
    void *mem = NULL;

    int required_blocks = (size + BALLOC_BLOCK_SIZE - 1) / BALLOC_BLOCK_SIZE;

    int run_start = 0;
    int run_size = 0;

    // Find a long enough chain of contiguous blocks.
    for (int i = 0; i < BALLOC_N_BLOCKS; i++)
    {
        if (allocated[i] == NULL)
        {
            // New run
            if (run_size == 0)
            {
                run_start = i;
            }
            // There is enough available memory
            if (++run_size == required_blocks)
            {
                break;
            }
        }
        else
        {
            // End of a run, reset
            run_start = 0;
            run_size = 0;
        }
    }

    if (run_size == required_blocks)
    {
        mem = mem_pool + run_start * BALLOC_BLOCK_SIZE;
        for (int i = 0; i < required_blocks; i++)
        {
            allocated[run_start + i] = mem;
        }
    }

    return mem;
}

void *balloc_bigblock(void)
{
    balloc_init();
    void *mem = NULL;

    int run_start = 0;
    int longest_start = 0;
    int run_size = 0;
    int longest_size = 0;

    // Find a long enough chain of contiguous blocks.
    for (int i = 0; i < BALLOC_N_BLOCKS; i++)
    {
        if (allocated[i] == NULL)
        {
            // New run
            if (run_size == 0)
            {
                run_start = i;
            }
            run_size++;

            if (run_size > longest_size)
            {
                longest_size = run_size;
                longest_start = run_start;
            }
        }
        else
        {
            run_start = 0;
            run_size = 0;
        }
    }

    if (longest_size != 0)
    {
        mem = mem_pool + longest_start * BALLOC_BLOCK_SIZE;
        for (int i = 0; i < longest_size; i++)
        {
            allocated[longest_start + i] = mem;
        }
    }

    return mem;
}

void bfree(void *mem)
{
    for (int i = 0; i < BALLOC_N_BLOCKS; i++)
    {
        if (mem == allocated[i])
        {
            allocated[i] = NULL;
        }
    }
}
