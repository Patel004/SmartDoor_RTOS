/* ═══════════════════════════════════════════════════════════════════════
 *  uart.c — USART2 driver with TX and RX
 *  PA2 = TX (AF7), PA3 = RX (AF7)
 *  9600 baud @ 4 MHz PCLK1 → BRR = 417
 * ═══════════════════════════════════════════════════════════════════════ */

#include "uart.h"

void UART2_Init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* PA2 TX, PA3 RX → AF mode */
    GPIOA->MODER  &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER  |=  (2U << (2*2)) | (2U << (3*2));

    /* AF7 = USART2 */
    GPIOA->AFR[0] &= ~((0xFU << (2*4)) | (0xFU << (3*4)));
    GPIOA->AFR[0] |=  (7U << (2*4)) | (7U << (3*4));

    /* 9600 baud @ 4 MHz → BRR = 417 */
    USART2->BRR = 417;

    /* Enable TX, RX, USART */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART2_SendChar(char c) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = (uint8_t)c;
}

void UART2_sendString(const char *str) {
    while (*str) UART2_SendChar(*str++);
}

/* Non-blocking receive — returns 1 and fills *c if a byte is ready */
uint8_t UART2_ReceiveChar(char *c) {
    if (USART2->ISR & USART_ISR_RXNE) {
        *c = (char)(USART2->RDR & 0xFF);
        return 1;
    }
    return 0;
}
