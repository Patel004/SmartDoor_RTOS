/* ═══════════════════════════════════════════════════════════════════════
 *  uart.h — USART2 driver with mutex-protected output
 *
 *  UART2_sendString() and UART2_SendChar() are safe to call from
 *  multiple FreeRTOS tasks — protected by xUARTMutex.
 * ═══════════════════════════════════════════════════════════════════════ */

#ifndef UART_H
#define UART_H

#include "stm32l476xx.h"
#include <stdint.h>

/* ── Public API ─────────────────────────────────────────────────── */
void    UART2_Init(void);
void    UART2_SendChar(char c);
void    UART2_sendString(const char *str);
uint8_t UART2_ReceiveChar(char *c);   /* Non-blocking RX */

/* ── Mutex-protected versions (use from tasks) ──────────────────── */
void    UART2_Print(const char *str);       /* Mutex-locked sendString */
void    UART2_PrintChar(char c);            /* Mutex-locked SendChar   */

#endif /* UART_H */
