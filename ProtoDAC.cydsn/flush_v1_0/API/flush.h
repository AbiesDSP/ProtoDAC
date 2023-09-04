#pragma once

#include "`$INSTANCE_NAME`_defs.h"
    
#include <stdint.h>
#include <cytypes.h>

static inline void `$INSTANCE_NAME`_Init(uint16_t transfer_size, uint16_t timeout)
{
    CY_SET_REG8(`$INSTANCE_NAME`_Flush_D0_PTR, transfer_size);
    CY_SET_REG8(`$INSTANCE_NAME`_Flush_A0_PTR, 0);
    CY_SET_REG8(`$INSTANCE_NAME`_Flush_D1_PTR, timeout);
    CY_SET_REG8(`$INSTANCE_NAME`_Flush_A1_PTR, timeout);
}

static inline uint8_t `$INSTANCE_NAME`_TransferSize(void)
{
    return CY_GET_REG8(`$INSTANCE_NAME`_Flush_F1_PTR);
}
