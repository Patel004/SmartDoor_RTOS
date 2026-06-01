/* ═══════════════════════════════════════════════════════════════════════
 *  SmartDoor RTOS — main.c FINAL
 * ═══════════════════════════════════════════════════════════════════════ */

#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart.h"
#include "lcd.h"
#include "keypad.h"
#include "RFID.h"
#include "rtc.h"
#include "flash_log.h"
#include "app_config.h"

QueueHandle_t xQueueRFID    = NULL;
QueueHandle_t xQueueKeypad  = NULL;
QueueHandle_t xQueueAuth    = NULL;
QueueHandle_t xQueueKeyChar = NULL;

void vTaskRFID   (void *pvParameters);
void vTaskKeypad (void *pvParameters);
void vTaskAuth   (void *pvParameters);
void vTaskLCD    (void *pvParameters);
void vTaskCLI    (void *pvParameters);

int main(void) {

    UART2_Init();
    UART2_sendString("\r\n=== SmartDoor RTOS FINAL ===\r\n");

    I2C1_Init();
    LCD_Init();
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("  Scan card or  ");
    LCD_SetCursor(1, 0);
    LCD_Print("  Enter PIN:    ");

    Keypad_Init();
    SPI_Init();
    RTC_Init();
    FlashLog_Init();

    xQueueRFID    = xQueueCreate(QUEUE_RFID_LEN,   sizeof(RFIDCard_t));
    xQueueKeypad  = xQueueCreate(QUEUE_KEYPAD_LEN,  PIN_LENGTH + 1);
    xQueueAuth    = xQueueCreate(QUEUE_AUTH_LEN,    sizeof(AuthEvent_t));
    xQueueKeyChar = xQueueCreate(PIN_LENGTH + 2,    sizeof(char));

    xTaskCreate(vTaskRFID,   "RFID",   STACK_RFID,   NULL, PRIORITY_RFID,   NULL);
    xTaskCreate(vTaskKeypad, "Keypad", STACK_KEYPAD, NULL, PRIORITY_KEYPAD, NULL);
    xTaskCreate(vTaskAuth,   "Auth",   STACK_AUTH,   NULL, PRIORITY_AUTH,   NULL);
    xTaskCreate(vTaskLCD,    "LCD",    STACK_LCD,    NULL, PRIORITY_LCD,    NULL);
    xTaskCreate(vTaskCLI,    "CLI",    STACK_CLI,    NULL, PRIORITY_CLI,    NULL);

    vTaskStartScheduler();
    for (;;);
}
