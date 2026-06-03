/* ═══════════════════════════════════════════════════════════════════════
 *  rtc.c — Bare-metal RTC driver for STM32L476
 *
 *  Uses LSI (32 kHz internal oscillator) as clock source.
 *  Checks if RTC is already running before reinitializing —
 *  this preserves the time across soft resets and power cycles
 *  as long as VDD is maintained.
 * ═══════════════════════════════════════════════════════════════════════ */

#include "rtc.h"

/* ─────────────────────────────────────────────────────────────────────
 *  BCD helpers
 * ───────────────────────────────────────────────────────────────────── */
static uint8_t dec_to_bcd(uint8_t dec) {
    return (uint8_t)(((dec / 10U) << 4U) | (dec % 10U));
}

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4U) * 10U) + (bcd & 0x0FU));
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC INIT
 *  Only resets and reinitializes if RTC has not been set before.
 *  If INITS flag is set in ISR, RTC is already initialized — skip.
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_Init(void) {

    /* Enable PWR clock, disable backup domain write protection */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1      |= PWR_CR1_DBP;

    /* Enable LSI */
    RCC->CSR |= RCC_CSR_LSION;
    while (!(RCC->CSR & RCC_CSR_LSIRDY));

    /* Check if RTC is already initialized (INITS flag in ISR) */
    if (RCC->BDCR & RCC_BDCR_RTCEN) {
        /* RTC clock already enabled — check if time is valid */
        if (RTC->ISR & RTC_ISR_INITS) {
            /* RTC already initialized — keep existing time */
            return;
        }
    }

    /* First time init — reset backup domain and configure */
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    /* Select LSI as RTC clock source */
    RCC->BDCR |= RCC_BDCR_RTCSEL_1;

    /* Enable RTC clock */
    RCC->BDCR |= RCC_BDCR_RTCEN;

    /* Unlock RTC write protection */
    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    /* Enter init mode */
    RTC->ISR |= RTC_ISR_INIT;
    while (!(RTC->ISR & RTC_ISR_INITF));

    /* Prescaler for LSI: async=127, sync=249 → 1 Hz */
    RTC->PRER = (127U << 16U) | 249U;

    /* Default time: 00:00:00 on 2025-01-01 */
    RTC->TR = 0x00000000U;
    RTC->DR = (0x25U << 16U) | (0x01U << 8U) | 0x01U;

    /* 24-hour format */
    RTC->CR &= ~RTC_CR_FMT;

    /* Exit init mode */
    RTC->ISR &= ~RTC_ISR_INIT;

    /* Re-lock */
    RTC->WPR = 0xFFU;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC GET TIME — read TR then DR (reading TR latches DR)
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_GetTime(RTC_Time_t *t) {
    uint32_t tr = RTC->TR;
    uint32_t dr = RTC->DR;

    t->hour   = bcd_to_dec((uint8_t)((tr >> 16U) & 0x3FU));
    t->minute = bcd_to_dec((uint8_t)((tr >>  8U) & 0x7FU));
    t->second = bcd_to_dec((uint8_t)( tr         & 0x7FU));
    t->year   = bcd_to_dec((uint8_t)((dr >> 16U) & 0xFFU));
    t->month  = bcd_to_dec((uint8_t)((dr >>  8U) & 0x1FU));
    t->day    = bcd_to_dec((uint8_t)( dr         & 0x3FU));
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC SET TIME — called from CLI set_time command
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_SetTime(const RTC_Time_t *t) {
    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    RTC->ISR |= RTC_ISR_INIT;
    while (!(RTC->ISR & RTC_ISR_INITF));

    RTC->TR = ((uint32_t)dec_to_bcd(t->hour)   << 16U)
            | ((uint32_t)dec_to_bcd(t->minute)  <<  8U)
            |  (uint32_t)dec_to_bcd(t->second);

    RTC->DR = ((uint32_t)dec_to_bcd(t->year)    << 16U)
            | ((uint32_t)dec_to_bcd(t->month)   <<  8U)
            |  (uint32_t)dec_to_bcd(t->day);

    RTC->ISR &= ~RTC_ISR_INIT;
    RTC->WPR  = 0xFFU;
}
