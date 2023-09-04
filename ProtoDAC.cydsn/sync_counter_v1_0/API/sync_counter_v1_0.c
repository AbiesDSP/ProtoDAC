#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_defs.h"

void `$INSTANCE_NAME`_start(void)
{
    `$INSTANCE_NAME`_SofCtr_Start();
    `$INSTANCE_NAME`_DP_F0_CLEAR;
}

uint32_t `$INSTANCE_NAME`_read()
{
    return CY_GET_REG24(`$INSTANCE_NAME`_DP_F0_PTR);
}
