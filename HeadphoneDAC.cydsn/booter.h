#pragma once
#include <stdint.h>
#include "avril.h"

extern AvrilInterface BooterIface;

void Booter(void *pvParameters);
void enter_bootload(void);
