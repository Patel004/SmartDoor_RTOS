/* ═══════════════════════════════════════════════════════════════════════
 *  lcd.c — HD44780 via I2C PCF8574
 *  Uses scheduler-safe delays — works both before and after scheduler.
 * ═══════════════════════════════════════════════════════════════════════ */

#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"

/* ─────────────────────────────────────────────────────────────────────
 *  Busy-wait delays — safe before AND after scheduler starts.
 *  At 4 MHz: 4 NOPs ≈ 1 µs, 4000 loops ≈ 1 ms
 * ───────────────────────────────────────────────────────────────────── */
static void lcd_delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us * 4; i++) __NOP();
}

static void lcd_delay_ms(uint32_t ms) {
    /* Use vTaskDelay if scheduler running, else busy-wait */
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        for (uint32_t i = 0; i < ms * 4000; i++) __NOP();
    }
}

static uint8_t backlight_state = LCD_BL;

/* ═══════════════════════════════════════════════════════════════════════
 *  I2C1 INIT — PB8=SCL, PB9=SDA, 4MHz → 100kHz
 * ═══════════════════════════════════════════════════════════════════════ */
void I2C1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    GPIOB->MODER  &= ~((3U<<(8*2))|(3U<<(9*2)));
    GPIOB->MODER  |=  (2U<<(8*2))|(2U<<(9*2));
    GPIOB->OTYPER |=  (1U<<8)|(1U<<9);
    GPIOB->OSPEEDR|=  (3U<<(8*2))|(3U<<(9*2));
    GPIOB->PUPDR  &= ~((3U<<(8*2))|(3U<<(9*2)));
    GPIOB->PUPDR  |=  (1U<<(8*2))|(1U<<(9*2));
    GPIOB->AFR[1] &= ~((0xFU<<0)|(0xFU<<4));
    GPIOB->AFR[1] |=  (4U<<0)|(4U<<4);

    I2C1->CR1     = 0;
    I2C1->TIMINGR = 0x00420F13;
    I2C1->CR1    |= I2C_CR1_PE;
}

static void PCF8574_Write(uint8_t data) {
    while (I2C1->ISR & I2C_ISR_BUSY);
    I2C1->CR2 = ((LCD_I2C_ADDR<<1) & I2C_CR2_SADD)
              | (1U<<I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND
              | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = data;
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR |= I2C_ICR_STOPCF;
}

static void LCD_PulseEnable(uint8_t data_byte) {
    PCF8574_Write(data_byte | LCD_EN);
    lcd_delay_us(1);
    PCF8574_Write(data_byte & ~LCD_EN);
    lcd_delay_us(50);
}

static void LCD_SendNibble(uint8_t nibble, uint8_t rs_flag) {
    uint8_t data = (nibble<<4) | backlight_state | rs_flag;
    LCD_PulseEnable(data);
}

static void LCD_SendByte(uint8_t byte, uint8_t rs_flag) {
    LCD_SendNibble(byte>>4,   rs_flag);
    LCD_SendNibble(byte&0x0F, rs_flag);
}

static void LCD_Command(uint8_t cmd) {
    LCD_SendByte(cmd, 0);
    if (cmd <= 0x03) lcd_delay_ms(2);
}

void LCD_SendChar(char c) {
    LCD_SendByte((uint8_t)c, LCD_RS);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  LCD INIT — works before AND after scheduler
 * ═══════════════════════════════════════════════════════════════════════ */
void LCD_Init(void) {
    lcd_delay_ms(50);
    LCD_SendNibble(0x03, 0); lcd_delay_ms(5);
    LCD_SendNibble(0x03, 0); lcd_delay_ms(1);
    LCD_SendNibble(0x03, 0); lcd_delay_ms(1);
    LCD_SendNibble(0x02, 0); lcd_delay_ms(1);
    LCD_Command(0x28);
    LCD_Command(0x08);
    LCD_Command(0x01); lcd_delay_ms(2);
    LCD_Command(0x06);
    LCD_Command(0x0C);
}

void LCD_Clear(void) {
    LCD_Command(0x01);
    lcd_delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    static const uint8_t row_offsets[2] = {0x00, 0x40};
    LCD_Command(0x80 | (row_offsets[row&0x01] + (col&0x0F)));
}

void LCD_Print(const char *str) {
    while (*str) LCD_SendChar(*str++);
}

void LCD_BacklightOn(void) {
    backlight_state = LCD_BL;
    PCF8574_Write(LCD_BL);
}

void LCD_BacklightOff(void) {
    backlight_state = 0;
    PCF8574_Write(0);
}
