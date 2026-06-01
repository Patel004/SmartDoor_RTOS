#include "keypad.h"

/* ─────────────────────────────────────────────────────────────────────
 *  4×4 KEYPAD CHARACTER MAP
 *  Columns are driven LOW one at a time; rows are read.
 * ───────────────────────────────────────────────────────────────────── */
static const char keypad[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* ─────────────────────────────────────────────────────────────────────
 *  COLUMN HELPERS  (drive one column LOW, rest HIGH)
 * ───────────────────────────────────────────────────────────────────── */
static void col_drive_low(int col) {
    if (col == 0) GPIOB->ODR &= ~(1U << COL1_PIN); // PB10
    if (col == 1) GPIOA->ODR &= ~(1U << COL2_PIN); // PA8
    if (col == 2) GPIOA->ODR &= ~(1U << COL3_PIN); // PA9
    if (col == 3) GPIOC->ODR &= ~(1U << COL4_PIN); // PC7
}

static void col_drive_high(int col) {
    if (col == 0) GPIOB->ODR |= (1U << COL1_PIN);
    if (col == 1) GPIOA->ODR |= (1U << COL2_PIN);
    if (col == 2) GPIOA->ODR |= (1U << COL3_PIN);
    if (col == 3) GPIOC->ODR |= (1U << COL4_PIN);
}

static void all_cols_high(void) {
    GPIOB->ODR |= (1U << COL1_PIN);
    GPIOA->ODR |= (1U << COL2_PIN);
    GPIOA->ODR |= (1U << COL3_PIN);
    GPIOC->ODR |= (1U << COL4_PIN);
}

/* ─────────────────────────────────────────────────────────────────────
 *  ROW PORT / PIN LOOKUP
 * ───────────────────────────────────────────────────────────────────── */
static const uint8_t row_pins[4]  = { ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN };
/*
 * Row port map: ROW1=PA10, ROW2=PB3, ROW3=PB5, ROW4=PB4
 * Stored as indices: 0=GPIOA, 1=GPIOB (cannot store pointers in const table
 * without casting tricks, so we resolve at runtime below).
 */
static GPIO_TypeDef * const row_ports[4] = { GPIOA, GPIOB, GPIOB, GPIOB };

/* ═══════════════════════════════════════════════════════════════════════
 *  KEYPAD INIT
 *
 *  BUG FIX: Original code called PUPDR |= without first clearing the
 *  two bits for each pin.  If the reset value of PUPDR was non-zero for
 *  any of those fields, a spurious pull-down could remain active,
 *  causing missed or phantom key presses.
 * ═══════════════════════════════════════════════════════════════════════ */
void Keypad_Init(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN
                  | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN;

    /* ── ROW pins: input mode, pull-up ─────────────────────────────── */

    /* ROW1 – PA10 */
    GPIOA->MODER  &= ~(3U << (ROW1_PIN * 2)); // Input (clear both bits)
    GPIOA->PUPDR  &= ~(3U << (ROW1_PIN * 2)); // Clear pull bits   (BUG FIX)
    GPIOA->PUPDR  |=  (1U << (ROW1_PIN * 2)); // Pull-up

    /* ROW2 – PB3 */
    GPIOB->MODER  &= ~(3U << (ROW2_PIN * 2));
    GPIOB->PUPDR  &= ~(3U << (ROW2_PIN * 2)); // BUG FIX
    GPIOB->PUPDR  |=  (1U << (ROW2_PIN * 2));

    /* ROW3 – PB5 */
    GPIOB->MODER  &= ~(3U << (ROW3_PIN * 2));
    GPIOB->PUPDR  &= ~(3U << (ROW3_PIN * 2)); // BUG FIX
    GPIOB->PUPDR  |=  (1U << (ROW3_PIN * 2));

    /* ROW4 – PB4 */
    GPIOB->MODER  &= ~(3U << (ROW4_PIN * 2));
    GPIOB->PUPDR  &= ~(3U << (ROW4_PIN * 2)); // BUG FIX
    GPIOB->PUPDR  |=  (1U << (ROW4_PIN * 2));

    /* ── COL pins: output mode, default HIGH ────────────────────────── */

    /* COL1 – PB10 */
    GPIOB->MODER  &= ~(3U << (COL1_PIN * 2));
    GPIOB->MODER  |=  (1U << (COL1_PIN * 2));

    /* COL2 – PA8 */
    GPIOA->MODER  &= ~(3U << (COL2_PIN * 2));
    GPIOA->MODER  |=  (1U << (COL2_PIN * 2));

    /* COL3 – PA9 */
    GPIOA->MODER  &= ~(3U << (COL3_PIN * 2));
    GPIOA->MODER  |=  (1U << (COL3_PIN * 2));

    /* COL4 – PC7 */
    GPIOC->MODER  &= ~(3U << (COL4_PIN * 2));
    GPIOC->MODER  |=  (1U << (COL4_PIN * 2));

    all_cols_high();
}

/* ═══════════════════════════════════════════════════════════════════════
 *  KEYPAD GET KEY
 *
 *  Drives each column LOW in turn, then reads all four row inputs.
 *  Returns the character from the map, or 0 if no key is pressed.
 *  Blocks until the key is released (debounce by waiting for release).
 * ═══════════════════════════════════════════════════════════════════════ */
char Keypad_GetKey(void) {
    for (int col = 0; col < 4; col++) {
        col_drive_low(col);

        for (int row = 0; row < 4; row++) {
            uint8_t      pin  = row_pins[row];
            GPIO_TypeDef *port = row_ports[row];

            if (!(port->IDR & (1U << pin))) {      // Row pulled LOW → key pressed
                /* Wait for key release before returning */
                while (!(port->IDR & (1U << pin)));

                col_drive_high(col);               // Restore column
                return keypad[row][col];
            }
        }

        col_drive_high(col); // Restore column before next iteration
    }
    return 0; // No key detected
}
