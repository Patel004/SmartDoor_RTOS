/* ═══════════════════════════════════════════════════════════════════════
 *  RFID.h — MFRC522 bare-metal driver for STM32L476
 *
 *  Uses SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI)
 *  CS  = PB6 (output, active low)
 *  RST = PB0 (output, active low)
 *  IRQ = PC0 (input, EXTI0, falling edge)
 * ═══════════════════════════════════════════════════════════════════════ */

#ifndef RFID_H
#define RFID_H

#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  Pin assignments
 * ───────────────────────────────────────────────────────────────────── */
#define RFID_CS_PORT        GPIOB
#define RFID_CS_PIN         6U

#define RFID_RST_PORT       GPIOB
#define RFID_RST_PIN        0U

#define RFID_IRQ_PORT       GPIOC
#define RFID_IRQ_PIN        0U

/* ─────────────────────────────────────────────────────────────────────
 *  MFRC522 register map (selected)
 * ───────────────────────────────────────────────────────────────────── */
#define CommandReg          0x01U
#define ComIEnReg           0x02U
#define DivIEnReg           0x03U
#define ComIrqReg           0x04U
#define ErrorReg            0x06U
#define FIFODataReg         0x09U
#define FIFOLevelReg        0x0AU
#define BitFramingReg       0x0DU
#define ModeReg             0x11U
#define TxControlReg        0x14U
#define TxASKReg            0x15U
#define TModeReg            0x2AU
#define TPrescalerReg       0x2BU
#define TReloadRegH         0x2CU
#define TReloadRegL         0x2DU
#define VersionReg          0x37U

/* ─────────────────────────────────────────────────────────────────────
 *  MFRC522 commands
 * ───────────────────────────────────────────────────────────────────── */
#define PCD_Idle            0x00U
#define PCD_Transceive      0x0CU

/* ─────────────────────────────────────────────────────────────────────
 *  PICC commands
 * ───────────────────────────────────────────────────────────────────── */
#define PICC_REQA           0x26U
#define PICC_ANTICOLL       0x93U

/* ─────────────────────────────────────────────────────────────────────
 *  Status codes
 * ───────────────────────────────────────────────────────────────────── */
#define STATUS_OK           0U
#define STATUS_ERROR        1U
#define STATUS_TIMEOUT      2U

/* ─────────────────────────────────────────────────────────────────────
 *  Card UID structure
 *  uid_size: number of valid bytes in uid[] (typically 4 or 7)
 *  valid:    set to 1 after successful anticollision
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t uid[7];        /* Up to 7-byte UID                        */
    uint8_t uid_size;      /* Number of valid UID bytes                */
    uint8_t valid;         /* 1 = UID successfully read                */
} RFIDCard_t;

/* ─────────────────────────────────────────────────────────────────────
 *  Semaphore — given by EXTI0 ISR, taken by vTaskRFID
 *  Declared extern so EXTI0_IRQHandler in RFID.c can give it,
 *  and task_rfid.c can take it.
 * ───────────────────────────────────────────────────────────────────── */
extern SemaphoreHandle_t xRFIDSemaphore;

/* ─────────────────────────────────────────────────────────────────────
 *  Public API
 * ───────────────────────────────────────────────────────────────────── */
void    SPI_Init(void);
uint8_t SPI_TransmitReceive(uint8_t data);

void    RFID_Init(void);
void    RFID_WriteRegister(uint8_t reg, uint8_t value);
uint8_t RFID_ReadRegister(uint8_t reg);
uint8_t RFID_DetectCard(RFIDCard_t *card);

/* Sub-ms busy-wait for SPI timing */
void    delay_us(uint32_t us);

#endif /* RFID_H */
