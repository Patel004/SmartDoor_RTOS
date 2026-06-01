#ifndef FLASH_LOG_H
#define FLASH_LOG_H

#include "stm32l476xx.h"
#include "rtc.h"
#include "app_config.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  STM32L476 Flash layout
 *  Total: 1 MB (2 banks × 512 KB, each bank has 256 pages of 2 KB)
 *  We use the LAST page of bank 1 for log storage.
 *  This keeps it far from our program code which starts at 0x08000000.
 *
 *  Last page of bank 1:
 *    Page 255 → 0x0807F800 to 0x0807FFFF (2 KB)
 * ───────────────────────────────────────────────────────────────────── */
#define LOG_PAGE_ADDR       0x0807F800UL   /* Start of log page          */
#define LOG_PAGE_SIZE       2048U          /* 2 KB page                  */
#define LOG_PAGE_NUMBER     255U           /* Page index in bank 1       */

/* ─────────────────────────────────────────────────────────────────────
 *  Log entry structure
 *  Each entry is 16 bytes — fits neatly in flash double-word writes.
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t     granted;        /* 1 = granted, 0 = denied              */
    uint8_t     source;         /* 0 = RFID, 1 = PIN                    */
    uint8_t     uid[4];         /* Card UID (zeros if PIN entry)        */
    RTC_Time_t  timestamp;      /* When it happened                     */
    uint8_t     reserved[3];    /* Pad to 16 bytes total                */
} LogEntry_t;

/* Max entries that fit in one 2 KB page */
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
