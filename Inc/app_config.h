#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ─── Access credentials ────────────────────────────────────────── */
#define PIN_LENGTH          4
#define CORRECT_PIN         "123A"

#define AUTH_UID_0          0x9C
#define AUTH_UID_1          0x31
#define AUTH_UID_2          0xAE
#define AUTH_UID_3          0x02

/* ─── Timing ────────────────────────────────────────────────────── */
#define RFID_POLL_MS            50
#define KEYPAD_POLL_MS          50
#define LCD_RESULT_MS           1200
#define CARD_COOLDOWN_MS        1500
#define CARD_NEW_COOLDOWN_MS    500  /* Increased from 200ms to prevent overflow */

/* ─── Task priorities ───────────────────────────────────────────── */
#define PRIORITY_RFID       2
#define PRIORITY_KEYPAD     2
#define PRIORITY_AUTH       3
#define PRIORITY_LCD        2
#define PRIORITY_CLI        1

/* ─── Stack sizes ───────────────────────────────────────────────── */
#define STACK_RFID          256
#define STACK_KEYPAD        128
#define STACK_AUTH          256
#define STACK_LCD           256
#define STACK_CLI           256

/* ─── Queue sizes — increased to handle rapid taps ─────────────── */
#define QUEUE_RFID_LEN      8    /* Was 4 — now handles burst of 8 */
#define QUEUE_KEYPAD_LEN    8
#define QUEUE_AUTH_LEN      8    /* Was 4 — LCD gets more results  */

/* ─── Auth event ────────────────────────────────────────────────── */
typedef enum {
    AUTH_GRANTED = 0,
    AUTH_DENIED
} AuthResult_t;

typedef struct {
    AuthResult_t result;
    char         source[8];
} AuthEvent_t;

#endif /* APP_CONFIG_H */
