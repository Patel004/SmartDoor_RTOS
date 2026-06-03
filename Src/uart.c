/* ═══════════════════════════════════════════════════════════════════════
 *  uart.c — USART2 bare-metal driver with FreeRTOS mutex protection
 *
 *  PA2 = TX (AF7), PA3 = RX (AF7)
 *  9600 baud @ 4 MHz PCLK1 → BRR = 417
 *
 *  UART2_Print() / UART2_PrintChar() are mutex-protected — safe to
 *  call from any task without data races on the TX register.
 *
 *  UART2_sendString() / UART2_SendChar() are raw (no mutex) — use
 *  only from main() before scheduler starts, or from ISR context.
 * ═══════════════════════════════════════════════════════════════════════ */

#include "uart.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ─────────────────────────────────────────────────────────────────────
 *  Mutex handle — created in UART2_Init() after scheduler starts.
 *  Declared here, accessible via extern if needed.
 * ───────────────────────────────────────────────────────────────────── */
SemaphoreHandle_t xUARTMutex = NULL;

/* ═══════════════════════════════════════════════════════════════════════
 *  UART2 INIT
 *  Configures GPIO and USART2. Safe to call before scheduler.
 *  Mutex is created here — call after scheduler starts OR recreate
 *  in first task. We create it here so main() can print boot messages.
 * ═══════════════════════════════════════════════════════════════════════ */
void UART2_Init(void) {
    /* Enable clocks */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* PA2 = TX, PA3 = RX → Alternate Function mode */
    GPIOA->MODER  &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
    GPIOA->MODER  |=  (2U << (2U * 2U)) | (2U << (3U * 2U));

    /* AF7 = USART2 on PA2 and PA3 */
    GPIOA->AFR[0] &= ~((0xFU << (2U * 4U)) | (0xFU << (3U * 4U)));
    GPIOA->AFR[0] |=  (7U  << (2U * 4U)) | (7U  << (3U * 4U));

    /* BRR = 417 → 9600 baud at 4 MHz */
    USART2->BRR = 417U;

    /* Enable TX, RX, USART */
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* Create mutex — safe to call before scheduler (mutex just allocates) */
    xUARTMutex = xSemaphoreCreateMutex();
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RAW TX — no mutex. Use from main() before scheduler or from ISR.
 * ═══════════════════════════════════════════════════════════════════════ */
void UART2_SendChar(char c) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = (uint8_t)c;
}

void UART2_sendString(const char *str) {
    while (*str) UART2_SendChar(*str++);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  NON-BLOCKING RX — returns 1 if byte received, 0 if empty
 * ═══════════════════════════════════════════════════════════════════════ */
uint8_t UART2_ReceiveChar(char *c) {
    if (USART2->ISR & USART_ISR_RXNE) {
        *c = (char)(USART2->RDR & 0xFFU);
        return 1U;
    }
    return 0U;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MUTEX-PROTECTED TX — safe to call from any FreeRTOS task.
 *  Acquires mutex, prints, releases mutex.
 *  Timeout: 10ms — if UART is busy, gives up rather than blocking forever.
 * ═══════════════════════════════════════════════════════════════════════ */
void UART2_Print(const char *str) {
    if (xUARTMutex == NULL) {
        UART2_sendString(str);  /* Fallback if mutex not yet created */
        return;
    }
    if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        UART2_sendString(str);
        xSemaphoreGive(xUARTMutex);
    }
}

void UART2_PrintChar(char c) {
    if (xUARTMutex == NULL) {
        UART2_SendChar(c);
        return;
    }
    if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        UART2_SendChar(c);
        xSemaphoreGive(xUARTMutex);
    }
}
