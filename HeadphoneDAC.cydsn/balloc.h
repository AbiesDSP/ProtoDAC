#pragma once
#include <stddef.h>

#define BALLOC_BLOCK_SIZE 1024
#define BALLOC_N_BLOCKS 8

void *balloc(size_t size);
// Reserve as much memory as possible.
void *balloc_bigblock(void);
void bfree(void *mem);
