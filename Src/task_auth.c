/* ═══════════════════════════════════════════════════════════════════════
 *  task_auth.c — uses flat LogEntry_t with direct RTC field assignment
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "RFID.h"
#include "uart.h"
#include "rtc.h"
#include "flash_log.h"
#include "app_config.h"
#include <string.h>

extern QueueHandle_t xQueueRFID;
extern QueueHandle_t xQueueKeypad;
extern QueueHandle_t xQueueAuth;

static const uint8_t auth_uid[4] = {
    AUTH_UID_0, AUTH_UID_1, AUTH_UID_2, AUTH_UID_3
};

static void print_two_digits(uint8_t val) {
    val = val % 100U;
    char buf[3];
    buf[0] = (char)('0' + (val / 10U));
    buf[1] = (char)('0' + (val % 10U));
    buf[2] = '\0';
    UART2_Print(buf);
}

static void print_timestamp(const LogEntry_t *e) {
    UART2_Print("[20");
    print_two_digits(e->year);
    UART2_Print("-");
    print_two_digits(e->month);
    UART2_Print("-");
    print_two_digits(e->day);
    UART2_Print(" ");
    print_two_digits(e->hour);
    UART2_Print(":");
    print_two_digits(e->minute);
    UART2_Print(":");
    print_two_digits(e->second);
    UART2_Print("]\r\n");
}

static void fill_timestamp(LogEntry_t *entry) {
    RTC_Time_t t = {0};
    RTC_GetTime(&t);
    entry->hour   = t.hour;
    entry->minute = t.minute;
    entry->second = t.second;
    entry->day    = t.day;
    entry->month  = t.month;
    entry->year   = t.year;
}

void vTaskAuth(void *pvParameters) {
    (void)pvParameters;

    /* Print RTC on boot for debug */
    RTC_Time_t dbg = {0};
    RTC_GetTime(&dbg);
    UART2_Print("[AUTH] Task started. RTC: [20");
    char buf[4];
    buf[0]=(char)('0'+(dbg.year/10));buf[1]=(char)('0'+(dbg.year%10));buf[2]='\0';
    UART2_Print(buf);
    UART2_Print("-");
    buf[0]=(char)('0'+(dbg.month/10));buf[1]=(char)('0'+(dbg.month%10));
    UART2_Print(buf);
    UART2_Print("-");
    buf[0]=(char)('0'+(dbg.day/10));buf[1]=(char)('0'+(dbg.day%10));
    UART2_Print(buf);
    UART2_Print(" ");
    buf[0]=(char)('0'+(dbg.hour/10));buf[1]=(char)('0'+(dbg.hour%10));
    UART2_Print(buf);
    UART2_Print(":");
    buf[0]=(char)('0'+(dbg.minute/10));buf[1]=(char)('0'+(dbg.minute%10));
    UART2_Print(buf);
    UART2_Print(":");
    buf[0]=(char)('0'+(dbg.second/10));buf[1]=(char)('0'+(dbg.second%10));
    UART2_Print(buf);
    UART2_Print("]\r\n");

    for (;;) {
        AuthEvent_t event     = {0};
        LogEntry_t  log_entry = {0};
        uint8_t     got_event = 0U;

        /* Check RFID queue */
        RFIDCard_t card = {0};
        if (xQueueReceive(xQueueRFID, &card, 0U) == pdTRUE) {
            uint8_t match = 1U;
            for (uint8_t i = 0U; i < 4U && i < card.uid_size; i++)
                if (card.uid[i] != auth_uid[i]) { match = 0U; break; }

            event.result = match ? AUTH_GRANTED : AUTH_DENIED;
            strncpy(event.source, "RFID", sizeof(event.source) - 1U);
            log_entry.granted = match;
            log_entry.source  = 0U;
            memcpy(log_entry.uid, card.uid, 4U);
            fill_timestamp(&log_entry);
            got_event = 1U;

            UART2_Print(match ? "[AUTH] RFID GRANTED " : "[AUTH] RFID DENIED ");
            print_timestamp(&log_entry);
        }

        /* Check Keypad queue */
        char pin[PIN_LENGTH + 1U] = {0};
        if (!got_event && xQueueReceive(xQueueKeypad, pin, 0U) == pdTRUE) {
            uint8_t match = (strncmp(pin, CORRECT_PIN, PIN_LENGTH) == 0) ? 1U : 0U;
            event.result = match ? AUTH_GRANTED : AUTH_DENIED;
            strncpy(event.source, "PIN", sizeof(event.source) - 1U);
            log_entry.granted = match;
            log_entry.source  = 1U;
            memset(log_entry.uid, 0U, sizeof(log_entry.uid));
            fill_timestamp(&log_entry);
            got_event = 1U;

            UART2_Print(match ? "[AUTH] PIN GRANTED " : "[AUTH] PIN DENIED ");
            print_timestamp(&log_entry);
        }

        if (got_event) {
            FlashLog_Write(&log_entry);
            xQueueSend(xQueueAuth, &event, 0U);
        }

        vTaskDelay(pdMS_TO_TICKS(20U));
    }
}
