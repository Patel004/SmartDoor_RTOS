/* ═══════════════════════════════════════════════════════════════════════
 *  task_cli.c — UART Command Line Interface
 *
 *  Commands:
 *    dump_log          Print all stored access events
 *    clear_log         Erase flash log
 *    set_time HH MM SS Set RTC time
 *    status            Show system status
 *    help              List commands
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "rtc.h"
#include "flash_log.h"
#include "watchdog.h"
#include "app_config.h"
#include <string.h>
#include <stdlib.h>

#define CLI_BUF_SIZE    64

/* ─────────────────────────────────────────────────────────────────────
 *  Print a single log entry over UART
 * ───────────────────────────────────────────────────────────────────── */
static void print_log_entry(uint32_t idx, const LogEntry_t *e) {
    /* Index */
    char num[8];
    uint32_t n = idx + 1;
    uint8_t  i = 6;
    num[7] = '\0';
    do { num[--i] = '0' + (n % 10); n /= 10; } while (n);
    UART2_sendString(&num[i]);
    UART2_sendString(". [20");

    /* Timestamp */
    UART2_SendChar('0' + e->timestamp.year   / 10);
    UART2_SendChar('0' + e->timestamp.year   % 10);
    UART2_SendChar('-');
    UART2_SendChar('0' + e->timestamp.month  / 10);
    UART2_SendChar('0' + e->timestamp.month  % 10);
    UART2_SendChar('-');
    UART2_SendChar('0' + e->timestamp.day    / 10);
    UART2_SendChar('0' + e->timestamp.day    % 10);
    UART2_SendChar(' ');
    UART2_SendChar('0' + e->timestamp.hour   / 10);
    UART2_SendChar('0' + e->timestamp.hour   % 10);
    UART2_SendChar(':');
    UART2_SendChar('0' + e->timestamp.minute / 10);
    UART2_SendChar('0' + e->timestamp.minute % 10);
    UART2_SendChar(':');
    UART2_SendChar('0' + e->timestamp.second / 10);
    UART2_SendChar('0' + e->timestamp.second % 10);
    UART2_sendString("] ");

    /* Result */
    UART2_sendString(e->granted ? "GRANTED " : "DENIED  ");

    /* Source */
    if (e->source == 0) {
        UART2_sendString("RFID ");
        /* Print UID */
        for (uint8_t j = 0; j < 4; j++) {
            uint8_t b = e->uid[j];
            char hi = (b >> 4) < 10 ? '0'+(b>>4) : 'A'+(b>>4)-10;
            char lo = (b & 0xF) < 10 ? '0'+(b&0xF) : 'A'+(b&0xF)-10;
            UART2_SendChar(hi);
            UART2_SendChar(lo);
            if (j < 3) UART2_SendChar(':');
        }
    } else {
        UART2_sendString("PIN");
    }
    UART2_sendString("\r\n");
}

/* ─────────────────────────────────────────────────────────────────────
 *  Command handlers
 * ───────────────────────────────────────────────────────────────────── */
static void cmd_dump_log(void) {
    uint32_t count = FlashLog_Count();
    if (count == 0) {
        UART2_sendString("Log is empty.\r\n");
        return;
    }
    UART2_sendString("--- Audit log (");
    char buf[8];
    utoa(count, buf, 10);
    UART2_sendString(buf);
    UART2_sendString(" entries) ---\r\n");

    for (uint32_t i = 0; i < count; i++) {
        LogEntry_t entry;
        if (FlashLog_Read(i, &entry)) {
            print_log_entry(i, &entry);
        }
        IWDG_Pet();  /* Pet watchdog while printing long logs */
    }
    UART2_sendString("--- End of log ---\r\n");
}

static void cmd_clear_log(void) {
    FlashLog_Clear();
    UART2_sendString("Log cleared.\r\n");
}

static void cmd_status(void) {
    RTC_Time_t t;
    RTC_GetTime(&t);
    UART2_sendString("System: SmartDoor RTOS v2.2\r\n");
    UART2_sendString("Time: 20");
    UART2_SendChar('0' + t.year   / 10); UART2_SendChar('0' + t.year   % 10);
    UART2_SendChar('-');
    UART2_SendChar('0' + t.month  / 10); UART2_SendChar('0' + t.month  % 10);
    UART2_SendChar('-');
    UART2_SendChar('0' + t.day    / 10); UART2_SendChar('0' + t.day    % 10);
    UART2_SendChar(' ');
    UART2_SendChar('0' + t.hour   / 10); UART2_SendChar('0' + t.hour   % 10);
    UART2_SendChar(':');
    UART2_SendChar('0' + t.minute / 10); UART2_SendChar('0' + t.minute % 10);
    UART2_SendChar(':');
    UART2_SendChar('0' + t.second / 10); UART2_SendChar('0' + t.second % 10);
    UART2_sendString("\r\n");
    UART2_sendString("Log entries: ");
    char buf[8];
    utoa(FlashLog_Count(), buf, 10);
    UART2_sendString(buf);
    UART2_sendString("\r\n");
}

static void cmd_set_time(const char *args) {
    /* Format: HH MM SS  e.g. "14 32 07" */
    RTC_Time_t t = {0};
    t.hour   = atoi(args);
    const char *p = args;
    while (*p && *p != ' ') p++;
    if (*p) { p++; t.minute = atoi(p); }
    while (*p && *p != ' ') p++;
    if (*p) { p++; t.second = atoi(p); }

    /* Keep existing date */
    RTC_Time_t cur;
    RTC_GetTime(&cur);
    t.day = cur.day; t.month = cur.month; t.year = cur.year;

    RTC_SetTime(&t);
    UART2_sendString("Time set.\r\n");
}

static void cmd_help(void) {
    UART2_sendString("Commands:\r\n");
    UART2_sendString("  dump_log          Print all log entries\r\n");
    UART2_sendString("  clear_log         Erase flash log\r\n");
    UART2_sendString("  set_time HH MM SS Set clock (24h)\r\n");
    UART2_sendString("  status            System info\r\n");
    UART2_sendString("  help              Show this list\r\n");
}

/* ─────────────────────────────────────────────────────────────────────
 *  Process a complete command line
 * ───────────────────────────────────────────────────────────────────── */
static void process_command(char *line) {
    /* Strip trailing whitespace */
    int len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' ||
                       line[len-1] == '\n' ||
                       line[len-1] == ' ')) {
        line[--len] = '\0';
    }

    if (len == 0) return;

    UART2_sendString("\r\n");

    if (strcmp(line, "dump_log") == 0) {
        cmd_dump_log();
    } else if (strcmp(line, "clear_log") == 0) {
        cmd_clear_log();
    } else if (strcmp(line, "status") == 0) {
        cmd_status();
    } else if (strncmp(line, "set_time ", 9) == 0) {
        cmd_set_time(line + 9);
    } else if (strcmp(line, "help") == 0) {
        cmd_help();
    } else {
        UART2_sendString("Unknown command. Type 'help'.\r\n");
    }

    UART2_sendString("> ");
}

/* ═══════════════════════════════════════════════════════════════════════
 *  CLI TASK
 * ═══════════════════════════════════════════════════════════════════════ */
void vTaskCLI(void *pvParameters) {
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(500));  /* Wait for other tasks to print first */

    UART2_sendString("\r\nType 'help' for commands.\r\n> ");

    char    buf[CLI_BUF_SIZE];
    uint8_t idx = 0;

    for (;;) {
        char c;
        if (UART2_ReceiveChar(&c)) {
            if (c == '\r' || c == '\n') {
                /* Enter pressed — process command */
                buf[idx] = '\0';
                process_command(buf);
                idx = 0;
            } else if (c == 0x7F || c == '\b') {
                /* Backspace */
                if (idx > 0) {
                    idx--;
                    UART2_sendString("\b \b");
                }
            } else if (idx < CLI_BUF_SIZE - 1) {
                /* Normal character — echo and buffer */
                buf[idx++] = c;
                UART2_SendChar(c);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
