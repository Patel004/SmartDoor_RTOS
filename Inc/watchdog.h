#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "stm32l476xx.h"
#include <stdint.h>

void     IWDG_Init(void);
void     IWDG_Pet(void);
uint8_t  IWDG_WasResetCause(void);

#endif
