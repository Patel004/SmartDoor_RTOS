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

    /* Hardware already init'd in main() */
    UART2_sendString("[KEYPAD] Task started\r\n");

    char    pin_buf[PIN_LENGTH + 1] = {0};
    uint8_t pin_idx = 0;

    for (;;) {
        char key = Keypad_GetKey();

        if (key != 0) {
            UART2_SendChar('*');

            if (pin_idx < PIN_LENGTH) {
                pin_buf[pin_idx] = key;
                pin_idx++;
                if (xQueueKeyChar != NULL)
                    xQueueSend(xQueueKeyChar, &key, 0);
            }

            if (pin_idx == PIN_LENGTH) {
                pin_buf[PIN_LENGTH] = '\0';
                UART2_sendString("\r\n[KEYPAD] PIN entered\r\n");
                xQueueSend(xQueueKeypad, pin_buf, 0);

                char clear = 0xFF;
                xQueueSend(xQueueKeyChar, &clear, 0);

                pin_idx = 0;
                memset(pin_buf, 0, sizeof(pin_buf));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(KEYPAD_POLL_MS));
    }
}
