/* ═══════════════════════════════════════════════════════════════════════
 *  SmartDoor RTOS — main.c FINAL
 *
 *  RTOS synchronization primitives used:
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │  xUARTMutex     — mutual exclusion on UART2 TX register     │
 *  │  xRFIDSemaphore — binary semaphore, ISR → RFID task wakeup  │
 *  │  xQueueRFID     — RFIDCard_t: RFID task → Auth task         │
 *  │  xQueueKeypad   — char[5]: Keypad task → Auth task          │
 *  │  xQueueAuth     — AuthEvent_t: Auth task → LCD task         │
 *  │  xQueueKeyChar  — char: Keypad task → LCD task (live *)     │
 *  └─────────────────────────────────────────────────────────────┘
 * ═══════════════════════════════════════════════════════════════════════ */

#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "uart.h"
#include "lcd.h"
#include "keypad.h"
#include "RFID.h"
#include "rtc.h"
#include "flash_log.h"
#include "app_config.h"

/* ─────────────────────────────────────────────────────────────────────
 *  Queue handles — all defined here, extern in task files
 * ───────────────────────────────────────────────────────────────────── */
QueueHandle_t xQueueRFID    = NULL;
QueueHandle_t xQueueKeypad  = NULL;
QueueHandle_t xQueueAuth    = NULL;
QueueHandle_t xQueueKeyChar = NULL;

/* ─────────────────────────────────────────────────────────────────────
 *  Task prototypes
 * ───────────────────────────────────────────────────────────────────── */
void vTaskRFID   (void *pvParameters);
void vTaskKeypad (void *pvParameters);
void vTaskAuth   (void *pvParameters);
void vTaskLCD    (void *pvParameters);
void vTaskCLI    (void *pvParameters);

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════════════════ */
int main(void) {

    /* ── Init hardware before scheduler ──────────────────────────────
     *  LCD_Init() uses scheduler-safe delay (busy-wait before start).
     *  All peripherals initialized here so tasks start clean.
     * ────────────────────────────────────────────────────────────────── */
    UART2_Init();   /* Also creates xUARTMutex */
    UART2_sendString("\r\n=== SmartDoor RTOS ===\r\n");
    UART2_sendString("[BOOT] UART ok\r\n");

    I2C1_Init();
    LCD_Init();
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("  Scan card or  ");
    LCD_SetCursor(1, 0);
    LCD_Print("  Enter PIN:    ");
    UART2_sendString("[BOOT] LCD ok\r\n");

    Keypad_Init();
    UART2_sendString("[BOOT] Keypad ok\r\n");

    SPI_Init();
    UART2_sendString("[BOOT] SPI ok\r\n");

    RTC_Init();
    FlashLog_Init();
    UART2_sendString("[BOOT] RTC + Flash ok\r\n");

    /* ── Create queues ───────────────────────────────────────────────── */
    xQueueRFID    = xQueueCreate(QUEUE_RFID_LEN,   sizeof(RFIDCard_t));
    xQueueKeypad  = xQueueCreate(QUEUE_KEYPAD_LEN,  PIN_LENGTH + 1U);
    xQueueAuth    = xQueueCreate(QUEUE_AUTH_LEN,    sizeof(AuthEvent_t));
    xQueueKeyChar = xQueueCreate(PIN_LENGTH + 2U,   sizeof(char));
    UART2_sendString("[BOOT] Queues ok\r\n");

    /* ── Create tasks ────────────────────────────────────────────────── */
    xTaskCreate(vTaskRFID,   "RFID",   STACK_RFID,   NULL, PRIORITY_RFID,   NULL);
    xTaskCreate(vTaskKeypad, "Keypad", STACK_KEYPAD, NULL, PRIORITY_KEYPAD, NULL);
    xTaskCreate(vTaskAuth,   "Auth",   STACK_AUTH,   NULL, PRIORITY_AUTH,   NULL);
    xTaskCreate(vTaskLCD,    "LCD",    STACK_LCD,    NULL, PRIORITY_LCD,    NULL);
    xTaskCreate(vTaskCLI,    "CLI",    STACK_CLI,    NULL, PRIORITY_CLI,    NULL);
    UART2_sendString("[BOOT] Tasks created\r\n");

    UART2_sendString("[BOOT] Starting scheduler...\r\n");
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);
}
