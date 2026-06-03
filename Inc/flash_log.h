#ifndef FLASH_LOG_H
#define FLASH_LOG_H

#include "stm32l476xx.h"
#include "rtc.h"
#include "app_config.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  Flash page for log storage
 *  Last page of bank 1: 0x0807F800 (2 KB)
 * ───────────────────────────────────────────────────────────────────── */
#define LOG_PAGE_ADDR       0x0807F800UL
#define LOG_PAGE_SIZE       2048U
#define LOG_PAGE_NUMBER     255U

/* ─────────────────────────────────────────────────────────────────────
 *  Log entry — exactly 16 bytes for clean 64-bit flash writes
 *
 *  Layout (verified with offsetof):
 *  [0]     granted   (1 byte)
 *  [1]     source    (1 byte)
 *  [2-5]   uid       (4 bytes)
 *  [6]     hour      (1 byte)
 *  [7]     minute    (1 byte)
 *  [8]     second    (1 byte)
 *  [9]     day       (1 byte)
 *  [10]    month     (1 byte)
 *  [11]    year      (1 byte)
 *  [12-15] reserved  (4 bytes) ← fixed from 3 to 4 to reach 16 bytes
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t    granted;        /* 1 = granted, 0 = denied    */
    uint8_t    source;         /* 0 = RFID,    1 = PIN        */
    uint8_t    uid[4];         /* Card UID (zeros for PIN)    */
    uint8_t    hour;           /* Flattened RTC time fields   */
    uint8_t    minute;         /* — avoids struct-in-struct   */
    uint8_t    second;         /* alignment issues            */
    uint8_t    day;
    uint8_t    month;
    uint8_t    year;
    uint8_t    reserved[4];    /* Pad to exactly 16 bytes     */
} LogEntry_t;

#define LOG_MAX_ENTRIES     (LOG_PAGE_SIZE / sizeof(LogEntry_t))

/* ─────────────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────────────── */
void     FlashLog_Init(void);
void     FlashLog_Write(const LogEntry_t *entry);
uint32_t FlashLog_Count(void);
uint8_t  FlashLog_Read(uint32_t index, LogEntry_t *entry);
void     FlashLog_Clear(void);

#endif /* FLASH_LOG_H */
