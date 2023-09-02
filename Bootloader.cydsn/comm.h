#pragma once
#include <cytypes.h>

void CyBtldrCommStart(void);
void CyBtldrCommStop (void);
void CyBtldrCommReset(void);
cystatus CyBtldrCommWrite(uint8* buffer, uint16 size, uint16* count, uint8 timeOut);
cystatus CyBtldrCommRead (uint8* buffer, uint16 size, uint16* count, uint8 timeOut);
