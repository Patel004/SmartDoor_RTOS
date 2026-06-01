#ifndef UART_H
#define UART_H

#include "stm32l476xx.h"

void    UART2_Init(void);
void    UART2_sendString(const char *str);
void    UART2_SendChar(char c);
uint8_t UART2_ReceiveChar(char *c);   /* Non-blocking — returns 1 if char received */

#endif
