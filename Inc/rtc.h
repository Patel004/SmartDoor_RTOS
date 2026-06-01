#ifndef RTC_H
#define RTC_H

#include "stm32l476xx.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  RTC timestamp structure
 *  Holds human-readable date and time after reading from RTC registers
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t hour;    /* 0-23 */
    uint8_t minute;  /* 0-59 */
    uint8_t second;  /* 0-59 */
    uint8_t day;     /* 1-31 */
    uint8_t month;   /* 1-12 */
    uint8_t year;    /* 0-99 (offset from 2000, so 25 = 2025) */
} RTC_Time_t;

/* ─────────────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────────────── */
void RTC_Init(void);
void RTC_GetTime(RTC_Time_t *t);
void RTC_SetTime(const RTC_Time_t *t);

#endif /* RTC_H */
