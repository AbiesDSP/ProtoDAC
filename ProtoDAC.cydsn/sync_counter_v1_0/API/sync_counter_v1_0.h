#pragma once
#include <stdint.h>

// Start the sof ctr
void `$INSTANCE_NAME`_start(void);

// Read the last clock counter value
uint32_t `$INSTANCE_NAME`_read();
