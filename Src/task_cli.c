/* ═══════════════════════════════════════════════════════════════════════
 *  task_cli.c — UART CLI using flat LogEntry_t fields
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "rtc.h"
#include "flash_log.h"
#include "app_config.h"
#include <string.h>
#include <stdlib.h>

#define CLI_BUF_SIZE    64

static char nibble_to_hex(uint8_t n) {
    n &= 0x0FU;
    return (n < 10U) ? (char)('0' + n) : (char)('A' + n - 10U);
}

static void print_two(uint8_t val) {
    val = val % 100U;
    char b[3];
    b[0]=(char)('0'+(val/10U));
    b[1]=(char)('0'+(val%10U));
    b[2]='\0';
    UART2_Print(b);
}

static void print_log_entry(uint32_t idx, const LogEntry_t *e) {
    /* Index number */
    char num[6];
    uint32_t n = idx + 1U;
    uint8_t  i = 4U;
    num[5] = '\0';
    do { num[--i] = (char)('0' + (n % 10U)); n /= 10U; } while (n);
    UART2_Print(&num[i]);
    UART2_Print(". [20");
    print_two(e->year);  UART2_Print("-");
    print_two(e->month); UART2_Print("-");
    print_two(e->day);   UART2_Print(" ");
    print_two(e->hour);  UART2_Print(":");
    print_two(e->minute);UART2_Print(":");
    print_two(e->second);UART2_Print("] ");

    UART2_Print(e->granted ? "GRANTED " : "DENIED  ");

    if (e->source == 0U) {
        UART2_Print("RFID ");
        for (uint8_t j = 0U; j < 4U; j++) {
            char hb[3];
            hb[0]=nibble_to_hex(e->uid[j]>>4U);
            hb[1]=nibble_to_hex(e->uid[j]);
            hb[2]='\0';
            UART2_Print(hb);
            if (j < 3U) UART2_Print(":");
        }
    } else {
        UART2_Print("PIN");
    }
    UART2_Print("\r\n");
}

static void cmd_dump_log(void) {
    uint32_t count = FlashLog_Count();
    if (count == 0U) {
        UART2_Print("Log is empty.\r\n");
        return;
    }
    char buf[16];
    UART2_Print("--- Audit log (");
    utoa(count, buf, 10);
    UART2_Print(buf);
    UART2_Print(" entries) ---\r\n");
    for (uint32_t i = 0U; i < count; i++) {
        LogEntry_t entry = {0};
        if (FlashLog_Read(i, &entry)) {
            print_log_entry(i, &entry);
        }
    }
    UART2_Print("--- End of log ---\r\n");
}

static void cmd_clear_log(void) {
    FlashLog_Clear();
    UART2_Print("Log cleared.\r\n");
}

static void cmd_status(void) {
    RTC_Time_t t = {0};
    RTC_GetTime(&t);
    UART2_Print("System: SmartDoor RTOS\r\n");
    UART2_Print("Time: 20");
    char buf[4];
    buf[0]=(char)('0'+(t.year/10));buf[1]=(char)('0'+(t.year%10));buf[2]='\0';
    UART2_Print(buf); UART2_Print("-");
    buf[0]=(char)('0'+(t.month/10));buf[1]=(char)('0'+(t.month%10));
    UART2_Print(buf); UART2_Print("-");
    buf[0]=(char)('0'+(t.day/10));buf[1]=(char)('0'+(t.day%10));
    UART2_Print(buf); UART2_Print(" ");
    buf[0]=(char)('0'+(t.hour/10));buf[1]=(char)('0'+(t.hour%10));
    UART2_Print(buf); UART2_Print(":");
    buf[0]=(char)('0'+(t.minute/10));buf[1]=(char)('0'+(t.minute%10));
    UART2_Print(buf); UART2_Print(":");
    buf[0]=(char)('0'+(t.second/10));buf[1]=(char)('0'+(t.second%10));
    UART2_Print(buf); UART2_Print("\r\n");
    UART2_Print("Log entries: ");
    char lbuf[8];
    utoa(FlashLog_Count(), lbuf, 10);
    UART2_Print(lbuf);
    UART2_Print("\r\n");
}

static void cmd_set_time(const char *args) {
    RTC_Time_t t = {0};
    t.hour   = (uint8_t)atoi(args);
    const char *p = args;
    while (*p && *p != ' ') p++;
    if (*p) { p++; t.minute = (uint8_t)atoi(p); }
    while (*p && *p != ' ') p++;
    if (*p) { p++; t.second = (uint8_t)atoi(p); }
    RTC_Time_t cur = {0};
    RTC_GetTime(&cur);
    t.day = cur.day; t.month = cur.month; t.year = cur.year;
    RTC_SetTime(&t);
    UART2_Print("Time set.\r\n");
}

static void cmd_help(void) {
    UART2_Print("Commands:\r\n");
    UART2_Print("  dump_log          Print all log entries\r\n");
    UART2_Print("  clear_log         Erase flash log\r\n");
    UART2_Print("  set_time HH MM SS Set clock (24h)\r\n");
    UART2_Print("  status            System info\r\n");
    UART2_Print("  help              Show this list\r\n");
}

static void process_command(char *line) {
    int len = strlen(line);
    while (len > 0 && (line[len-1]=='\r'||line[len-1]=='\n'||line[len-1]==' '))
        line[--len] = '\0';
    if (len == 0) return;
    UART2_Print("\r\n");
    if      (strcmp(line, "dump_log")  == 0) cmd_dump_log();
    else if (strcmp(line, "clear_log") == 0) cmd_clear_log();
    else if (strcmp(line, "status")    == 0) cmd_status();
    else if (strcmp(line, "help")      == 0) cmd_help();
    else if (strncmp(line, "set_time ", 9) == 0) cmd_set_time(line + 9);
    else UART2_Print("Unknown command. Type 'help'.\r\n");
    UART2_Print("> ");
}

void vTaskCLI(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(500U));
    UART2_Print("\r\nType 'help' for commands.\r\n> ");

    char    buf[CLI_BUF_SIZE];
    uint8_t idx = 0U;
    memset(buf, 0, sizeof(buf));

    for (;;) {
        char c = 0;
        if (UART2_ReceiveChar(&c)) {
            if (c == '\r' || c == '\n') {
                buf[idx] = '\0';
                process_command(buf);
                idx = 0U;
                memset(buf, 0, sizeof(buf));
            } else if ((c == 0x7F || c == '\b') && idx > 0U) {
                idx--;
                UART2_Print("\b \b");
            } else if (idx < CLI_BUF_SIZE - 1U) {
                buf[idx++] = c;
                UART2_PrintChar(c);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}
