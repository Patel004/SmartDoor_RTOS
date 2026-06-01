/* ═══════════════════════════════════════════════════════════════════════
 *  rtc.c — Bare-metal RTC driver for STM32L476
 *
 *  Uses LSI (32 kHz internal oscillator) as clock source.
 *  No external crystal needed — works on any Nucleo board.
 *
 *  BCD format: STM32 RTC stores values in BCD (Binary Coded Decimal).
 *  Example: 14 (decimal) is stored as 0x14 in BCD.
 *  We convert between BCD and decimal on read/write.
 * ═══════════════════════════════════════════════════════════════════════ */

#include "rtc.h"

/* ─────────────────────────────────────────────────────────────────────
 *  BCD helpers
 * ───────────────────────────────────────────────────────────────────── */
static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC INIT
 *
 *  1. Enable PWR clock and disable backup domain write protection
 *  2. Enable LSI oscillator
 *  3. Select LSI as RTC clock source
 *  4. Enable RTC clock
 *  5. Unlock RTC write protection
 *  6. Enter init mode and set default time
 *  7. Exit init mode and re-lock
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_Init(void) {

    /* Step 1: Enable PWR clock, disable backup write protection */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1      |= PWR_CR1_DBP;          /* Disable backup domain protection */

    /* Step 2: Enable LSI (32 kHz internal oscillator) */
    RCC->CSR |= RCC_CSR_LSION;
    while (!(RCC->CSR & RCC_CSR_LSIRDY));  /* Wait for LSI ready */

    /* Step 3: Reset backup domain, select LSI as RTC source */
    RCC->BDCR |= RCC_BDCR_BDRST;          /* Reset backup domain */
    RCC->BDCR &= ~RCC_BDCR_BDRST;
    RCC->BDCR |= RCC_BDCR_RTCSEL_1;       /* 10 = LSI as RTC clock */

    /* Step 4: Enable RTC clock */
    RCC->BDCR |= RCC_BDCR_RTCEN;

    /* Step 5: Unlock RTC write protection (two-key sequence) */
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* Step 6: Enter init mode */
    RTC->ISR |= RTC_ISR_INIT;
    while (!(RTC->ISR & RTC_ISR_INITF));   /* Wait for init mode */

    /* Set prescaler for LSI: async=127, sync=249 → 1 Hz tick
     * LSI = 32000 Hz / (127+1) / (249+1) = 1.0 Hz               */
    RTC->PRER = (127U << 16) | 249U;

    /* Set default time: 00:00:00 on 01/01/2025 */
    RTC->TR = 0x00000000;                  /* 00:00:00 */
    RTC->DR = (0x25 << 16) | (0x01 << 8) | 0x01;  /* 2025-01-01 */

    /* 24-hour format */
    RTC->CR &= ~RTC_CR_FMT;

    /* Step 7: Exit init mode */
    RTC->ISR &= ~RTC_ISR_INIT;

    /* Re-lock write protection */
    RTC->WPR = 0xFF;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC GET TIME
 *  Reads current time and date from RTC registers.
 *  IMPORTANT: must read TR before DR (reading TR latches DR).
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_GetTime(RTC_Time_t *t) {
    uint32_t tr = RTC->TR;   /* Read time first — latches date */
    uint32_t dr = RTC->DR;

    t->hour   = bcd_to_dec((tr >> 16) & 0x3F);
    t->minute = bcd_to_dec((tr >>  8) & 0x7F);
    t->second = bcd_to_dec( tr        & 0x7F);

    t->year   = bcd_to_dec((dr >> 16) & 0xFF);
    t->month  = bcd_to_dec((dr >>  8) & 0x1F);
    t->day    = bcd_to_dec( dr        & 0x3F);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  RTC SET TIME
 *  Called from UART CLI: set_time command.
 *  t->year is offset from 2000 (25 = 2025).
 * ═══════════════════════════════════════════════════════════════════════ */
void RTC_SetTime(const RTC_Time_t *t) {
    /* Unlock */
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* Enter init mode */
    RTC->ISR |= RTC_ISR_INIT;
    while (!(RTC->ISR & RTC_ISR_INITF));

    RTC->TR = ((uint32_t)dec_to_bcd(t->hour)   << 16)
            | ((uint32_t)dec_to_bcd(t->minute)  <<  8)
            |  (uint32_t)dec_to_bcd(t->second);

    RTC->DR = ((uint32_t)dec_to_bcd(t->year)   << 16)
            | ((uint32_t)dec_to_bcd(t->month)   <<  8)
            |  (uint32_t)dec_to_bcd(t->day);

    /* Exit init mode */
    RTC->ISR &= ~RTC_ISR_INIT;

    /* Re-lock */
    RTC->WPR = 0xFF;
}
