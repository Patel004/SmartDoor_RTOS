/* ═══════════════════════════════════════════════════════════════════════
 *  task_auth.c — final, no watchdog
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

static void print_time(const RTC_Time_t *t) {
    char buf[20];
    buf[0]='2';buf[1]='0';
    buf[2]='0'+(t->year/10);buf[3]='0'+(t->year%10);buf[4]='-';
    buf[5]='0'+(t->month/10);buf[6]='0'+(t->month%10);buf[7]='-';
    buf[8]='0'+(t->day/10);buf[9]='0'+(t->day%10);buf[10]=' ';
    buf[11]='0'+(t->hour/10);buf[12]='0'+(t->hour%10);buf[13]=':';
    buf[14]='0'+(t->minute/10);buf[15]='0'+(t->minute%10);buf[16]=':';
    buf[17]='0'+(t->second/10);buf[18]='0'+(t->second%10);buf[19]='\0';
    UART2_sendString(buf);
}

void vTaskAuth(void *pvParameters) {
    (void)pvParameters;

    UART2_sendString("[AUTH] Task started\r\n");

    for (;;) {
        AuthEvent_t event     = {0};
        LogEntry_t  log_entry = {0};
        uint8_t     got_event = 0;

        RFIDCard_t card = {0};
        if (xQueueReceive(xQueueRFID, &card, 0) == pdTRUE) {
            uint8_t match = 1;
            for (uint8_t i = 0; i < 4 && i < card.uid_size; i++)
                if (card.uid[i] != auth_uid[i]) { match=0; break; }
            event.result = match ? AUTH_GRANTED : AUTH_DENIED;
            strncpy(event.source, "RFID", sizeof(event.source));
            log_entry.granted = match;
            log_entry.source  = 0;
            memcpy(log_entry.uid, card.uid, 4);
            RTC_GetTime(&log_entry.timestamp);
            got_event = 1;
            UART2_sendString("[AUTH] ");
            print_time(&log_entry.timestamp);
            UART2_sendString(match?" RFID GRANTED\r\n":" RFID DENIED\r\n");
        }

        char pin[PIN_LENGTH+1] = {0};
        if (!got_event && xQueueReceive(xQueueKeypad, pin, 0) == pdTRUE) {
            uint8_t match = (strncmp(pin, CORRECT_PIN, PIN_LENGTH) == 0);
            event.result = match ? AUTH_GRANTED : AUTH_DENIED;
            strncpy(event.source, "PIN", sizeof(event.source));
            log_entry.granted = match;
            log_entry.source  = 1;
            RTC_GetTime(&log_entry.timestamp);
            got_event = 1;
            UART2_sendString("[AUTH] ");
            print_time(&log_entry.timestamp);
            UART2_sendString(match?" PIN GRANTED\r\n":" PIN DENIED\r\n");
        }

        if (got_event) {
            FlashLog_Write(&log_entry);
            xQueueSend(xQueueAuth, &event, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
