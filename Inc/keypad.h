#ifndef KEYPAD_H
#define KEYPAD_H

#include "stm32l476xx.h"

/* ─────────────────────────────────────────────────────────────────────
 *  KEYPAD PIN ASSIGNMENTS
 *
 *  ROW pins are INPUTS (with internal pull-up).
 *  COL pins are OUTPUTS (driven LOW one at a time during scan).
 *
 *  Nucleo-L476RG Arduino headers:
 *    PA10 → D2   PA8 → D7   PA9 → D8
 *    PB3  → D3   PB10→ D6   PC7 → D9
 *    PB5  → D4
 *    PB4  → D5
 * ───────────────────────────────────────────────────────────────────── */
#define ROW1_PIN  10   // PA10  (D2)
#define ROW2_PIN   3   // PB3   (D3)
#define ROW3_PIN   5   // PB5   (D4)
#define ROW4_PIN   4   // PB4   (D5)

#define COL1_PIN  10   // PB10  (D6)
#define COL2_PIN   8   // PA8   (D7)
#define COL3_PIN   9   // PA9   (D8)
#define COL4_PIN   7   // PC7   (D9)

/* ─────────────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────────────── */
void Keypad_Init(void);
char Keypad_GetKey(void);

#endif // KEYPAD_H
