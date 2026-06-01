#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "RFID.h"
#include "uart.h"
#include "app_config.h"

extern QueueHandle_t xQueueRFID;

static char nibble_to_hex(uint8_t n) {
    n &= 0x0F;
    return (n < 10) ? ('0'+n) : ('A'+n-10);
}

static void print_uid(const RFIDCard_t *card) {
    UART2_sendString("Card UID: ");
    for (uint8_t i = 0; i < card->uid_size; i++) {
        UART2_SendChar(nibble_to_hex(card->uid[i]>>4));
        UART2_SendChar(nibble_to_hex(card->uid[i]));
        if (i < card->uid_size-1) UART2_SendChar(':');
    }
    UART2_sendString("\r\n");
}

static uint8_t same_uid(const RFIDCard_t *a, const RFIDCard_t *b) {
    if (a->uid_size != b->uid_size) return 0;
    for (uint8_t i = 0; i < a->uid_size; i++)
        if (a->uid[i] != b->uid[i]) return 0;
    return 1;
}

void vTaskRFID(void *pvParameters) {
    (void)pvParameters;

    /* Small delay then init RFID — SPI already init'd in main() */
    vTaskDelay(pdMS_TO_TICKS(200));
    RFID_Init();

    /* Print version to confirm SPI works */
    uint8_t ver = RFID_ReadRegister(VersionReg);
    UART2_sendString("[RFID] Version: 0x");
    UART2_SendChar(nibble_to_hex(ver>>4));
    UART2_SendChar(nibble_to_hex(ver));
    UART2_sendString("\r\n");
    UART2_sendString("[RFID] Task started\r\n");

    RFIDCard_t last_card     = {0};
    TickType_t last_card_tick = 0;

    for (;;) {
        RFIDCard_t card = {0};

        if (RFID_DetectCard(&card) == STATUS_OK) {
            TickType_t now      = xTaskGetTickCount();
            uint32_t   ms_since = (now - last_card_tick) * portTICK_PERIOD_MS;
            uint8_t    is_same  = same_uid(&card, &last_card);
            uint8_t    should_process = 0;

            if (last_card_tick == 0)                      should_process = 1;
            if (is_same  && ms_since >= CARD_COOLDOWN_MS) should_process = 1;
            if (!is_same && ms_since >= 1000)             should_process = 1;

            if (should_process) {
                last_card      = card;
                last_card_tick = now;
                print_uid(&card);
                xQueueSend(xQueueRFID, &card, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(RFID_POLL_MS));
    }
}
