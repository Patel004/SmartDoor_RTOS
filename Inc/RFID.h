#ifndef RFID_H
#define RFID_H

#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  PIN ASSIGNMENTS
 * ───────────────────────────────────────────────────────────────────── */
#define RFID_CS_PORT    GPIOB
#define RFID_CS_PIN     6

#define RFID_RST_PORT   GPIOB
#define RFID_RST_PIN    0

#define RFID_IRQ_PORT   GPIOC
#define RFID_IRQ_PIN    0

/* ─────────────────────────────────────────────────────────────────────
 *  MFRC522 REGISTER MAP
 * ───────────────────────────────────────────────────────────────────── */
#define CommandReg      0x01
#define ComIEnReg       0x02
#define DivIEnReg       0x03
#define ComIrqReg       0x04
#define DivIrqReg       0x05
#define ErrorReg        0x06
#define FIFODataReg     0x09
#define FIFOLevelReg    0x0A
#define BitFramingReg   0x0D
#define ModeReg         0x11
#define TxControlReg    0x14
#define TxASKReg        0x15
#define CRCResultRegH   0x21
#define CRCResultRegL   0x22
#define TModeReg        0x2A
#define TPrescalerReg   0x2B
#define TReloadRegH     0x2C
#define TReloadRegL     0x2D
#define VersionReg      0x37

/* ─────────────────────────────────────────────────────────────────────
 *  MFRC522 COMMANDS
 * ───────────────────────────────────────────────────────────────────── */
#define PCD_Idle        0x00
#define PCD_CalcCRC     0x03
#define PCD_Transmit    0x04
#define PCD_Receive     0x08
#define PCD_Transceive  0x0C
#define PCD_SoftReset   0x0F

/* ─────────────────────────────────────────────────────────────────────
 *  PICC COMMANDS
 * ───────────────────────────────────────────────────────────────────── */
#define PICC_REQA       0x26
#define PICC_ANTICOLL   0x93

/* ─────────────────────────────────────────────────────────────────────
 *  STATUS CODES
 * ───────────────────────────────────────────────────────────────────── */
#define STATUS_OK       0
#define STATUS_ERROR    1
#define STATUS_TIMEOUT  2

/* ─────────────────────────────────────────────────────────────────────
 *  CARD DATA STRUCTURE
 * ───────────────────────────────────────────────────────────────────── */
typedef struct {
    uint8_t uid[5];
    uint8_t uid_size;
    uint8_t valid;
} RFIDCard_t;

/* ─────────────────────────────────────────────────────────────────────
 *  GLOBALS
 * ───────────────────────────────────────────────────────────────────── */
extern volatile uint8_t rfid_card_detected;
extern RFIDCard_t       detected_card;

/* ─────────────────────────────────────────────────────────────────────
 *  FUNCTION PROTOTYPES
 * ───────────────────────────────────────────────────────────────────── */
void    SPI_Init(void);
uint8_t SPI_TransmitReceive(uint8_t data);

void    RFID_Init(void);
void    RFID_Reset(void);
void    RFID_WriteRegister(uint8_t reg, uint8_t value);
uint8_t RFID_ReadRegister(uint8_t reg);
uint8_t RFID_DetectCard(RFIDCard_t *card);

/* delay_us still available for sub-ms hardware timing */
void    delay_us(uint32_t us);

#endif /* RFID_H */
