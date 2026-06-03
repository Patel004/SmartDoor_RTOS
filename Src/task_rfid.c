/* ═══════════════════════════════════════════════════════════════════════
 *  task_rfid.c — 50ms polling with queue-aware sending
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "RFID.h"
#include "uart.h"
#include "app_config.h"

extern QueueHandle_t xQueueRFID;

static char nibble_to_hex(uint8_t n) {
    n &= 0x0FU;
    return (n < 10U) ? (char)('0' + n) : (char)('A' + n - 10U);
}

static void print_uid(const RFIDCard_t *card) {
    char    buf[32];
    uint8_t idx = 0U;
    buf[idx++]='[';buf[idx++]='R';buf[idx++]='F';
    buf[idx++]='I';buf[idx++]='D';buf[idx++]=']';
    buf[idx++]=' ';buf[idx++]='U';buf[idx++]='I';
    buf[idx++]='D';buf[idx++]=':';buf[idx++]=' ';
    for (uint8_t i = 0U; i < card->uid_size; i++) {
        buf[idx++] = nibble_to_hex(card->uid[i] >> 4U);
        buf[idx++] = nibble_to_hex(card->uid[i]);
        if (i < card->uid_size - 1U) buf[idx++] = ':';
    }
    buf[idx++]='\r'; buf[idx++]='\n'; buf[idx]='\0';
    UART2_Print(buf);
}

static uint8_t uid_match(const RFIDCard_t *a, const RFIDCard_t *b) {
    if (a->uid_size != b->uid_size) return 0U;
    for (uint8_t i = 0U; i < a->uid_size; i++)
        if (a->uid[i] != b->uid[i]) return 0U;
    return 1U;
}

void vTaskRFID(void *pvParameters) {
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(200U));
    RFID_Init();

    uint8_t ver = RFID_ReadRegister(VersionReg);
    char ver_buf[20];
    ver_buf[0]='[';ver_buf[1]='R';ver_buf[2]='F';ver_buf[3]='I';
    ver_buf[4]='D';ver_buf[5]=']';ver_buf[6]=' ';ver_buf[7]='v';
    ver_buf[8]=nibble_to_hex(ver>>4U);
    ver_buf[9]=nibble_to_hex(ver);
    ver_buf[10]='\r';ver_buf[11]='\n';ver_buf[12]='\0';
    UART2_Print(ver_buf);
    UART2_Print("[RFID] Task ready\r\n");

    static RFIDCard_t last_card;
    static TickType_t last_tick;
    last_card.uid_size = 0U;
    last_card.valid    = 0U;
    last_tick          = 0U;

    for (;;) {
        RFIDCard_t card;
        card.uid_size = 0U;
        card.valid    = 0U;

        if (RFID_DetectCard(&card) == STATUS_OK) {

            TickType_t now      = xTaskGetTickCount();
            uint32_t   ms_since = (uint32_t)((now - last_tick)
                                   * portTICK_PERIOD_MS);
            uint8_t    is_same  = uid_match(&card, &last_card);
            uint8_t    process  = 0U;

            if (last_tick == 0U) {
                process = 1U;
            } else if (is_same) {
                if (ms_since >= CARD_COOLDOWN_MS) process = 1U;
            } else {
                if (ms_since >= CARD_NEW_COOLDOWN_MS) process = 1U;
            }

            if (process) {
                /* Only send if queue has space — prevents silent drops */
                UBaseType_t spaces = uxQueueSpacesAvailable(xQueueRFID);
                if (spaces > 0U) {
                    last_card = card;
                    last_tick = now;
                    print_uid(&card);
                    xQueueSend(xQueueRFID, &card, 0U);
                } else {
                    /* Queue full — update tick so cooldown resets */
                    UART2_Print("[RFID] Queue full — skipping\r\n");
                    last_tick = now;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(RFID_POLL_MS));
    }
}
