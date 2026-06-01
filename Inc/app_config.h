#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define PIN_LENGTH          4
#define CORRECT_PIN         "123A"

#define AUTH_UID_0          0x9C
#define AUTH_UID_1          0x31
#define AUTH_UID_2          0xAE
#define AUTH_UID_3          0x02

#define RFID_POLL_MS        100
#define KEYPAD_POLL_MS      50
#define LCD_RESULT_MS       2000
#define CARD_COOLDOWN_MS    3000

#define PRIORITY_RFID       2
#define PRIORITY_KEYPAD     2
#define PRIORITY_AUTH       3
#define PRIORITY_LCD        2
#define PRIORITY_CLI        1

#define STACK_RFID          256
#define STACK_KEYPAD        128
#define STACK_AUTH          256
#define STACK_LCD           256
#define STACK_CLI           256

#define QUEUE_RFID_LEN      4
#define QUEUE_KEYPAD_LEN    8
#define QUEUE_AUTH_LEN      4

typedef enum {
    AUTH_GRANTED = 0,
    AUTH_DENIED
} AuthResult_t;

typedef struct {
    AuthResult_t result;
    char         source[8];
} AuthEvent_t;

#endif /* APP_CONFIG_H */
