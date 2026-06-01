/* ═══════════════════════════════════════════════════════════════════════
 *  flash_log.c — Bare-metal flash audit log for STM32L476
 *
 *  Writes LogEntry_t structs sequentially into a dedicated flash page.
 *  When the page is full, it erases and starts again (circular).
 *
 *  STM32L476 flash write rules:
 *  1. Must unlock flash before writing (two-key sequence)
 *  2. Must erase a full page before writing new data
 *  3. Must write in 64-bit (double-word) chunks only
 *  4. Each address can only be written ONCE per erase cycle
 * ═══════════════════════════════════════════════════════════════════════ */

#include "flash_log.h"
#include <string.h>

/* ─────────────────────────────────────────────────────────────────────
 *  Current write position — index of next free slot
 * ───────────────────────────────────────────────────────────────────── */
static uint32_t log_index = 0;

/* ─────────────────────────────────────────────────────────────────────
 *  FLASH UNLOCK / LOCK
 * ───────────────────────────────────────────────────────────────────── */
static void flash_unlock(void) {
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123UL;
        FLASH->KEYR = 0xCDEF89ABUL;
    }
}

static void flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flash_wait_busy(void) {
    while (FLASH->SR & FLASH_SR_BSY);
}

/* ─────────────────────────────────────────────────────────────────────
 *  ERASE one page (2 KB)
 * ───────────────────────────────────────────────────────────────────── */
static void flash_erase_page(uint32_t page) {
    flash_wait_busy();

    /* Set page erase mode, select page, start erase */
    FLASH->CR |= FLASH_CR_PER;
    FLASH->CR &= ~FLASH_CR_PNB_Msk;
    FLASH->CR |= (page << FLASH_CR_PNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;

    flash_wait_busy();

    /* Clear page erase bit */
    FLASH->CR &= ~FLASH_CR_PER;
}

/* ─────────────────────────────────────────────────────────────────────
 *  WRITE 8 bytes (64-bit double-word) to flash address
 *  addr must be 8-byte aligned.
 * ───────────────────────────────────────────────────────────────────── */
static void flash_write_dword(uint32_t addr, uint64_t data) {
    flash_wait_busy();

    FLASH->CR |= FLASH_CR_PG;  /* Enable programming */

    /* Write low 32 bits first, then high 32 bits */
    *(__IO uint32_t *)addr         = (uint32_t)(data & 0xFFFFFFFF);
    *(__IO uint32_t *)(addr + 4)   = (uint32_t)(data >> 32);

    flash_wait_busy();

    FLASH->CR &= ~FLASH_CR_PG; /* Disable programming */
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FLASH LOG INIT
 *  Scans the log page to find where the last valid entry is.
 *  This way we survive power cuts — log continues where it left off.
 * ═══════════════════════════════════════════════════════════════════════ */
void FlashLog_Init(void) {
    log_index = 0;

    /* Scan forward until we find an empty slot (all 0xFF = erased flash) */
    for (uint32_t i = 0; i < LOG_MAX_ENTRIES; i++) {
        LogEntry_t *entry = (LogEntry_t *)(LOG_PAGE_ADDR +
                                           i * sizeof(LogEntry_t));
        /* Check if slot is empty — erased flash reads as 0xFF */
        uint8_t *b = (uint8_t *)entry;
        uint8_t empty = 1;
        for (uint8_t j = 0; j < sizeof(LogEntry_t); j++) {
            if (b[j] != 0xFF) { empty = 0; break; }
        }
        if (empty) {
            log_index = i;
            return;
        }
    }

    /* Page is full — erase and start over */
    flash_unlock();
    flash_erase_page(LOG_PAGE_NUMBER);
    flash_lock();
    log_index = 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FLASH LOG WRITE
 *  Writes one LogEntry_t to the next available flash slot.
 * ═══════════════════════════════════════════════════════════════════════ */
void FlashLog_Write(const LogEntry_t *entry) {
    if (log_index >= LOG_MAX_ENTRIES) {
        /* Page full — erase and wrap */
        flash_unlock();
        flash_erase_page(LOG_PAGE_NUMBER);
        flash_lock();
        log_index = 0;
    }

    uint32_t addr = LOG_PAGE_ADDR + log_index * sizeof(LogEntry_t);

    /* Entry is 16 bytes = 2 double-words */
    uint8_t buf[sizeof(LogEntry_t)];
    memcpy(buf, entry, sizeof(LogEntry_t));

    flash_unlock();

    /* Write 8 bytes at a time */
    for (uint32_t i = 0; i < sizeof(LogEntry_t); i += 8) {
        uint64_t dword;
        memcpy(&dword, buf + i, 8);
        flash_write_dword(addr + i, dword);
    }

    flash_lock();

    log_index++;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FLASH LOG COUNT
 *  Returns how many entries are currently stored.
 * ═══════════════════════════════════════════════════════════════════════ */
uint32_t FlashLog_Count(void) {
    return log_index;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FLASH LOG READ
 *  Reads one entry by index into caller's buffer.
 *  Returns 1 if valid, 0 if index out of range.
 * ═══════════════════════════════════════════════════════════════════════ */
uint8_t FlashLog_Read(uint32_t index, LogEntry_t *entry) {
    if (index >= log_index) return 0;
    uint32_t addr = LOG_PAGE_ADDR + index * sizeof(LogEntry_t);
    memcpy(entry, (void *)addr, sizeof(LogEntry_t));
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FLASH LOG CLEAR
 *  Erases the entire log page. Called from CLI: clear_log command.
 * ═══════════════════════════════════════════════════════════════════════ */
void FlashLog_Clear(void) {
    flash_unlock();
    flash_erase_page(LOG_PAGE_NUMBER);
    flash_lock();
    log_index = 0;
}
