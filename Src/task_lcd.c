/* ═══════════════════════════════════════════════════════════════════════
 *  task_lcd.c
 *  Hardware already initialized in main() — task just updates display.
 * ═══════════════════════════════════════════════════════════════════════ */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "uart.h"
#include "app_config.h"

extern QueueHandle_t xQueueAuth;
extern QueueHandle_t xQueueKeyChar;

static void show_idle(void) {
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("  Scan card or  ");
    LCD_SetCursor(1, 0);
    LCD_Print("  Enter PIN:    ");
}

static void show_granted(void) {
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("**ACCESS GRANTED");
    LCD_SetCursor(1, 0);
    LCD_Print(" Door Unlocked! ");
}

static void show_denied(void) {
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("**ACCESS DENIED*");
    LCD_SetCursor(1, 0);
    LCD_Print("  Try again...  ");
}

void vTaskLCD(void *pvParameters) {
    (void)pvParameters;

    /* Hardware already init'd in main() — just show idle screen */
    show_idle();
    UART2_sendString("[LCD] Task started\r\n");

    uint8_t star_count = 0;

    for (;;) {
        /* Check individual keypress for live * display */
        char key = 0;
        if (xQueueReceive(xQueueKeyChar, &key, 0) == pdTRUE) {
            if ((uint8_t)key == 0xFF) {
                star_count = 0;
            } else {
                LCD_SetCursor(1, 10 + star_count);
                LCD_SendChar('*');
                star_count++;
                if (star_count > PIN_LENGTH) star_count = PIN_LENGTH;
            }
        }

        /* Check for auth result */
        AuthEvent_t event = {0};
        if (xQueueReceive(xQueueAuth, &event,
                          pdMS_TO_TICKS(20)) == pdTRUE) {
            if (event.result == AUTH_GRANTED) {
                show_granted();
            } else {
                show_denied();
            }
            vTaskDelay(pdMS_TO_TICKS(LCD_RESULT_MS));
            star_count = 0;
            show_idle();
        }
    }
}
