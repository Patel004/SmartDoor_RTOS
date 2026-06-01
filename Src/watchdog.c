/* ═══════════════════════════════════════════════════════════════════════
 *  watchdog.c — IWDG with 10 second timeout
 *
 *  LSI = 32000 Hz
 *  Prescaler = 256 → tick = 8 ms
 *  Reload = 1250 → timeout = 1250 × 8 ms = 10 seconds
 * ═══════════════════════════════════════════════════════════════════════ */

#include "watchdog.h"

void IWDG_Init(void) {
    IWDG->KR  = 0x5555;
    IWDG->PR  = IWDG_PR_PR_2 | IWDG_PR_PR_1 | IWDG_PR_PR_0;
    IWDG->RLR = 1250;
    while (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU));
    IWDG->KR  = 0xCCCC;
}

void IWDG_Pet(void) {
    IWDG->KR = 0xAAAA;
}

uint8_t IWDG_WasResetCause(void) {
    uint8_t was_wdg = 0;
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        was_wdg = 1;
        RCC->CSR |= RCC_CSR_RMVF;
    }
    return was_wdg;
}
