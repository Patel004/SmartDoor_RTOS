#ifndef LCD_H
#define LCD_H

#include "stm32l476xx.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────
 *  HARDWARE CONNECTIONS
 *
 *  I2C1:  PB8 = SCL  (AF4)
 *         PB9 = SDA  (AF4)
 *
 *  LCD module: HD44780 + PCF8574 I2C backpack (very common blue/green
 *  16x2 LCD modules sold with an I2C adapter board).
 *
 *  PCF8574 → HD44780 wiring (standard backpack wiring):
 *    P0 → RS    (Register Select)
 *    P1 → RW    (Read/Write, always driven LOW here)
 *    P2 → EN    (Enable)
 *    P3 → BL    (Backlight transistor, HIGH = on)
 *    P4 → D4
 *    P5 → D5
 *    P6 → D6
 *    P7 → D7
 * ───────────────────────────────────────────────────────────────────── */

/* PCF8574 7-bit I2C address.
   Default solder-bridge address = 0x27.  Some modules use 0x3F. */
#define LCD_I2C_ADDR    0x27

/* Bit masks for the PCF8574 output byte */
#define LCD_RS          0x01
#define LCD_RW          0x02
#define LCD_EN          0x04
#define LCD_BL          0x08   // Backlight

/* ─────────────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────────────── */
void I2C1_Init(void);
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_SendChar(char c);
void LCD_BacklightOn(void);
void LCD_BacklightOff(void);

#endif // LCD_H
