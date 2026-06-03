/* ═══════════════════════════════════════════════════════════════════════
 *  task_keypad.c — matrix keypad scan with mutex-protected UART
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "keypad.h"
#include "uart.h"
#include "app_config.h"
#include <string.h>

extern QueueHandle_t xQueueKeypad;
extern QueueHandle_t xQueueKeyChar;

void vTaskKeypad(void *pvParameters) {
    (void)pvParameters;
    UART2_Print("[KEYPAD] Task started\r\n");

    static char    pin_buf[PIN_LENGTH + 1U];
    static uint8_t pin_idx;

    memset(pin_buf, 0, sizeof(pin_buf));
    pin_idx = 0U;

    for (;;) {
        char key = Keypad_GetKey();

        if (key != 0) {
            UART2_PrintChar('*');

            if (pin_idx < PIN_LENGTH) {
                pin_buf[pin_idx] = key;
                pin_idx++;
                if (xQueueKeyChar != NULL) {
                    xQueueSend(xQueueKeyChar, &key, 0U);
                }
            }

            if (pin_idx == PIN_LENGTH) {
                pin_buf[PIN_LENGTH] = '\0';
                UART2_Print("\r\n[KEYPAD] PIN entered\r\n");
                xQueueSend(xQueueKeypad, pin_buf, 0U);

                char clear = (char)0xFF;
                xQueueSend(xQueueKeyChar, &clear, 0U);

                pin_idx = 0U;
                memset(pin_buf, 0, sizeof(pin_buf));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(KEYPAD_POLL_MS));
    }
}
