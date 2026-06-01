#include "RFID.h"

/* ─────────────────────────────────────────────────────────────────────
 *  GLOBALS
 * ───────────────────────────────────────────────────────────────────── */
volatile uint8_t rfid_card_detected = 0;
RFIDCard_t       detected_card      = {0};

/* ═══════════════════════════════════════════════════════════════════════
 *  SPI INIT
 * ═══════════════════════════════════════════════════════════════════════ */
void SPI_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA5=SCK, PA6=MISO, PA7=MOSI (AF5) */
    GPIOA->MODER &= ~((3U << (5*2)) | (3U << (6*2)) | (3U << (7*2)));
    GPIOA->MODER |=  (2U << (5*2)) | (2U << (6*2)) | (2U << (7*2));
    GPIOA->OSPEEDR |= (3U << (5*2)) | (3U << (6*2)) | (3U << (7*2));
    GPIOA->AFR[0]  |= (5U << (5*4)) | (5U << (6*4)) | (5U << (7*4));

    /* CS: PB6 output, default HIGH */
    GPIOB->MODER &= ~(3U << (RFID_CS_PIN * 2));
    GPIOB->MODER |=  (1U << (RFID_CS_PIN * 2));
    RFID_CS_PORT->ODR |= (1U << RFID_CS_PIN);

    /* RST: PB0 output, default HIGH */
    GPIOB->MODER &= ~(3U << (RFID_RST_PIN * 2));
    GPIOB->MODER |=  (1U << (RFID_RST_PIN * 2));
    RFID_RST_PORT->ODR |= (1U << RFID_RST_PIN);

    /* SPI1 control registers */
    SPI1->CR1 = 0;
    SPI1->CR2 = SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2 | SPI_CR2_FRXTH;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void RFID_Cleanup(void) {
    RFID_WriteRegister(CommandReg,    PCD_Idle);
    RFID_WriteRegister(ComIrqReg,     0x7F);
    RFID_WriteRegister(FIFOLevelReg,  0x80);
    RFID_WriteRegister(BitFramingReg, 0x00);
}

/* ─────────────────────────────────────────────────────────────────────
 *  SPI TRANSMIT / RECEIVE
 * ───────────────────────────────────────────────────────────────────── */
uint8_t SPI_TransmitReceive(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *(volatile uint8_t *)&SPI1->DR;
}

/* ─────────────────────────────────────────────────────────────────────
 *  REGISTER ACCESS
 * ───────────────────────────────────────────────────────────────────── */
void RFID_WriteRegister(uint8_t reg, uint8_t value) {
    RFID_CS_PORT->ODR &= ~(1U << RFID_CS_PIN);
    SPI_TransmitReceive((reg << 1) & 0x7E);
    SPI_TransmitReceive(value);
    RFID_CS_PORT->ODR |= (1U << RFID_CS_PIN);
}

uint8_t RFID_ReadRegister(uint8_t reg) {
    uint8_t value;
    RFID_CS_PORT->ODR &= ~(1U << RFID_CS_PIN);
    SPI_TransmitReceive(((reg << 1) & 0x7E) | 0x80);
    value = SPI_TransmitReceive(0x00);
    RFID_CS_PORT->ODR |= (1U << RFID_CS_PIN);
    return value;
}

/* ─────────────────────────────────────────────────────────────────────
 *  HARDWARE RESET
 *  NOTE: uses vTaskDelay — must be called from a FreeRTOS task context
 * ───────────────────────────────────────────────────────────────────── */
void RFID_Reset(void) {
    RFID_RST_PORT->ODR &= ~(1U << RFID_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(10));
    RFID_RST_PORT->ODR |= (1U << RFID_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(50));
}

/* ─────────────────────────────────────────────────────────────────────
 *  IRQ PIN + EXTI INIT
 * ───────────────────────────────────────────────────────────────────── */
static void RFID_IRQ_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    GPIOC->MODER  &= ~(3U << (RFID_IRQ_PIN * 2));
    GPIOC->PUPDR  &= ~(3U << (RFID_IRQ_PIN * 2));
    GPIOC->PUPDR  |=  (1U << (RFID_IRQ_PIN * 2));

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~(0xFU << 0);
    SYSCFG->EXTICR[0] |=  (0x2U << 0);

    EXTI->IMR1  |=  (1U << RFID_IRQ_PIN);
    EXTI->FTSR1 |=  (1U << RFID_IRQ_PIN);
    EXTI->RTSR1 &= ~(1U << RFID_IRQ_PIN);

    /* Priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)
     * so FreeRTOS API calls inside the ISR are safe.                      */
    NVIC_SetPriority(EXTI0_IRQn, 5);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* ─────────────────────────────────────────────────────────────────────
 *  RFID INIT
 * ───────────────────────────────────────────────────────────────────── */
void RFID_Init(void) {
    RFID_Reset();

    RFID_WriteRegister(TModeReg,      0x8D);
    RFID_WriteRegister(TPrescalerReg, 0x3E);
    RFID_WriteRegister(TReloadRegH,   0x00);
    RFID_WriteRegister(TReloadRegL,   0x30);
    RFID_WriteRegister(TxASKReg,      0x40);
    RFID_WriteRegister(ModeReg,       0x3D);

    uint8_t txCtrl = RFID_ReadRegister(TxControlReg);
    if (!(txCtrl & 0x03)) {
        RFID_WriteRegister(TxControlReg, txCtrl | 0x03);
    }

    RFID_WriteRegister(ComIEnReg, 0xA0);
    RFID_WriteRegister(DivIEnReg, 0x80);

    RFID_IRQ_Init();
}

/* ─────────────────────────────────────────────────────────────────────
 *  REQA
 * ───────────────────────────────────────────────────────────────────── */
static uint8_t RFID_REQA(void) {
    RFID_WriteRegister(CommandReg,    PCD_Idle);
    RFID_WriteRegister(ComIrqReg,     0x7F);
    RFID_WriteRegister(FIFOLevelReg,  0x80);
    RFID_WriteRegister(CommandReg,    PCD_Idle);
    RFID_WriteRegister(FIFODataReg,   PICC_REQA);
    RFID_WriteRegister(BitFramingReg, 0x07);
    RFID_WriteRegister(CommandReg,    PCD_Transceive);
    RFID_WriteRegister(BitFramingReg, 0x87);

    uint8_t irqVal;
    uint32_t timeout = 5000;
    do {
        irqVal = RFID_ReadRegister(ComIrqReg);
        timeout--;
    } while (!(irqVal & 0x30) && timeout);

    RFID_WriteRegister(BitFramingReg, 0x00);

    if (!timeout)                              return STATUS_TIMEOUT;
    if (irqVal & 0x01)                         return STATUS_TIMEOUT;
    if (RFID_ReadRegister(ErrorReg) & 0x13)    return STATUS_ERROR;
    return STATUS_OK;
}

/* ─────────────────────────────────────────────────────────────────────
 *  ANTI-COLLISION
 * ───────────────────────────────────────────────────────────────────── */
static uint8_t RFID_Anticollision(RFIDCard_t *card) {
    uint8_t irqVal;
    uint32_t timeout = 5000;

    RFID_WriteRegister(ComIrqReg,     0x7F);
    RFID_WriteRegister(FIFOLevelReg,  0x80);
    RFID_WriteRegister(CommandReg,    PCD_Idle);
    RFID_WriteRegister(FIFODataReg,   PICC_ANTICOLL);
    RFID_WriteRegister(FIFODataReg,   0x20);
    RFID_WriteRegister(BitFramingReg, 0x00);
    RFID_WriteRegister(CommandReg,    PCD_Transceive);
    RFID_WriteRegister(BitFramingReg, 0x80);

    do {
        irqVal = RFID_ReadRegister(ComIrqReg);
        timeout--;
    } while (!(irqVal & 0x30) && timeout);

    RFID_WriteRegister(BitFramingReg, 0x00);

    if (!timeout || (irqVal & 0x01))           return STATUS_TIMEOUT;
    if (RFID_ReadRegister(ErrorReg)  & 0x13)   return STATUS_ERROR;

    uint8_t n = RFID_ReadRegister(FIFOLevelReg);
    if (n == 0 || n > 5) return STATUS_ERROR;

    card->uid_size = n;
    for (uint8_t i = 0; i < n; i++) {
        card->uid[i] = RFID_ReadRegister(FIFODataReg);
    }
    card->valid = 1;
    return STATUS_OK;
}

/* ─────────────────────────────────────────────────────────────────────
 *  PUBLIC: DETECT CARD
 * ───────────────────────────────────────────────────────────────────── */
uint8_t RFID_DetectCard(RFIDCard_t *card) {
    uint8_t result = RFID_REQA();
    if (result != STATUS_OK) {
        RFID_Cleanup();
        return STATUS_ERROR;
    }
    result = RFID_Anticollision(card);
    RFID_Cleanup();
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  EXTI0 IRQ HANDLER
 *  Priority set to 5 — safe to call FreeRTOS ISR API functions
 * ═══════════════════════════════════════════════════════════════════════ */
void EXTI0_IRQHandler(void) {
    if (EXTI->PR1 & (1U << RFID_IRQ_PIN)) {
        EXTI->PR1 |= (1U << RFID_IRQ_PIN);

        RFIDCard_t temp = {0};
        if (RFID_DetectCard(&temp) == STATUS_OK) {
            detected_card      = temp;
            rfid_card_detected = 1;
        }
        RFID_WriteRegister(ComIrqReg, 0x7F);
    }
}

/* ─────────────────────────────────────────────────────────────────────
 *  delay_us — simple NOP loop, still used for sub-millisecond waits
 *  inside SPI/I2C drivers before the scheduler starts.
 * ───────────────────────────────────────────────────────────────────── */
void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us * 4; i++) {
        __NOP();
    }
}
