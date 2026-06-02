# SmartDoor RTOS — Production-Grade Access Control Firmware

> Bare-metal embedded firmware for the STM32L476RG (ARM Cortex-M4) implementing a multi-factor access control system with FreeRTOS preemptive scheduling, RTC-timestamped audit logging to internal flash, and a UART command-line interface — written entirely at the register level without STM32 HAL.

---

## What It Does

SmartDoor is a dual-authentication door access controller. A user can gain entry by either tapping an authorized RFID card or entering a 4-digit PIN on a matrix keypad. Every access attempt — granted or denied — is permanently logged to internal flash with a real-time timestamp and can be retrieved over serial at any time.

This is not a demo. Every design decision reflects how production embedded firmware is actually built: concurrent tasks instead of polling loops, non-volatile audit trails, field-configurable credentials, and fault recovery.

---

## Hardware

| Component | Part | Interface |
|---|---|---|
| Microcontroller | STM32L476RG (ARM Cortex-M4, 80 MHz, 1 MB Flash, 96 KB RAM) | — |
| RFID reader | MFRC522 | SPI1 (PA5/PA6/PA7, CS=PB6, RST=PB0) |
| Keypad | 4×4 matrix | GPIO (PA8/PA9/PA10, PB3/PB4/PB5/PB10, PC7) |
| Display | HD44780 16×2 LCD via PCF8574 I2C backpack | I2C1 (PB8/PB9) |
| Debug/CLI | UART terminal | USART2 (PA2/PA3, 9600 baud) |
| Clock | Internal RTC with LSI oscillator | RTC peripheral |

---

## System Architecture

```
main()
  ├── Hardware init (I2C, SPI, UART, RTC, Flash) — before scheduler
  ├── Queue creation (4 queues)
  └── vTaskStartScheduler()
        │
        ├── vTaskRFID   [Priority 2] — polls MFRC522 every 100ms
        │     └── xQueueSend(xQueueRFID, &card_uid)
        │
        ├── vTaskKeypad [Priority 2] — scans matrix every 50ms
        │     ├── xQueueSend(xQueueKeyChar, &key)   → LCD live update
        │     └── xQueueSend(xQueueKeypad, pin)     → auth on 4th key
        │
        ├── vTaskAuth   [Priority 3] — validates credentials
        │     ├── xQueueReceive(xQueueRFID)
        │     ├── xQueueReceive(xQueueKeypad)
        │     ├── RTC_GetTime() → timestamp
        │     ├── FlashLog_Write() → permanent record
        │     └── xQueueSend(xQueueAuth, &result)
        │
        ├── vTaskLCD    [Priority 2] — drives display
        │     ├── xQueueReceive(xQueueKeyChar) → show * per keypress
        │     └── xQueueReceive(xQueueAuth)   → show result 2s
        │
        └── vTaskCLI    [Priority 1] — UART command interface
              ├── dump_log   → print flash audit log
              ├── clear_log  → erase flash page
              ├── set_time   → configure RTC
              └── status     → system info
```

### Queue data flow

```
RFID task ──(RFIDCard_t)──► Auth task ──(AuthEvent_t)──► LCD task
Keypad task ─(char[5])────► Auth task
Keypad task ─(char)───────────────────────────────────► LCD task (live *)
```

---

## Firmware Features

### FreeRTOS preemptive scheduling
Five independent tasks run concurrently. No blocking waits in the main loop — each task sleeps with `vTaskDelay()` and yields the CPU to other tasks. The scheduler switches context every 1ms via SysTick.

### Bare-metal peripheral drivers (no HAL)
Every peripheral is configured by directly writing to hardware registers:
- SPI1 with correct 8-bit frame size (`DS[3:0]=0111`, `FRXTH=1`) and Mode 0
- I2C1 with computed TIMINGR for 100 kHz at 4 MHz PCLK1
- USART2 with BRR calculated for 9600 baud
- RTC with LSI oscillator, BCD time registers, write-protection sequence
- Flash with unlock key sequence, page erase, and 64-bit double-word writes

### RTC-timestamped audit log
Every access event is stored as a 16-byte `LogEntry_t` struct in a dedicated 2 KB flash page:

```c
typedef struct {
    uint8_t    granted;     // 1 = granted, 0 = denied
    uint8_t    source;      // 0 = RFID, 1 = PIN
    uint8_t    uid[4];      // card UID (zeros for PIN)
    RTC_Time_t timestamp;   // year/month/day/hour/min/sec
    uint8_t    reserved[3]; // pad to 16 bytes
} LogEntry_t;
```

The log survives power cycles. On boot, the driver scans for the last written entry and resumes from there. When full, it wraps by erasing and starting over.

### UART CLI
A command-line interface runs as a background task, parsing input character by character and responding to commands:

```
> set_time 14 32 00
Time set.

> dump_log
--- Audit log (7 entries) ---
1. [2025-01-01 14:32:09] GRANTED  RFID  9C:31:AE:02
2. [2025-01-01 14:32:18] DENIED   RFID  43:1E:24:03
3. [2025-01-01 14:32:24] DENIED   PIN
4. [2025-01-01 14:32:33] GRANTED  PIN
--- End of log ---

> status
System: SmartDoor RTOS v2.2
Time: 2025-01-01 14:33:27
Log entries: 7
```

### IWDG Watchdog driver
An independent watchdog driver (`watchdog.c`) is implemented and tested. The IWDG is configured for a 10-second timeout using the LSI oscillator at /256 prescaler. The Auth task pets the watchdog every 20ms during normal operation. The driver detects and reports watchdog-caused resets on boot via the RCC reset flag register.

---

## Project Structure

```
SmartDoor_RTOS/
├── Src/
│   ├── main.c          — hardware init + task creation
│   ├── task_rfid.c     — MFRC522 polling task
│   ├── task_keypad.c   — matrix keypad scan task
│   ├── task_auth.c     — credential validation + flash logging
│   ├── task_lcd.c      — HD44780 display task
│   ├── task_cli.c      — UART command parser task
│   ├── rfid.c          — SPI1 + MFRC522 register driver
│   ├── lcd.c           — I2C1 + PCF8574 + HD44780 driver
│   ├── keypad.c        — GPIO matrix scan driver
│   ├── uart.c          — USART2 TX/RX driver
│   ├── rtc.c           — RTC init, get, set (BCD conversion)
│   ├── flash_log.c     — internal flash page write/read/erase
│   └── watchdog.c      — IWDG init, pet, reset-cause detection
├── Inc/
│   ├── app_config.h    — PIN, UID, timing, stack sizes
│   ├── FreeRTOSConfig.h
│   ├── rfid.h / lcd.h / keypad.h / uart.h
│   ├── rtc.h / flash_log.h / watchdog.h
├── FreeRTOS/           — FreeRTOS kernel v11.3.0
│   ├── Source/         — tasks.c, queue.c, list.c, timers.c
│   └── Source/portable/GCC/ARM_CM4F/
├── CMSIS/              — ARM + ST device headers
└── STM32L476RGTX_FLASH.ld
```

---

## Build Instructions

**Toolchain:** STM32CubeIDE (GCC ARM, no HAL, bare-metal)

1. Clone the repository
2. Open STM32CubeIDE → File → Import → Existing Projects into Workspace
3. Select the `SmartDoor_RTOS` folder
4. Project → Build Project
5. Run → Run (flashes via ST-LINK on Nucleo board)

**Serial monitor:** Connect at 9600 baud on the ST-LINK virtual COM port. Use minicom, screen, or any serial terminal.

---

## Configuration

All credentials and timing are in `Inc/app_config.h`:

```c
#define CORRECT_PIN     "123A"       // 4-character PIN

#define AUTH_UID_0      0x9C         // authorized RFID card UID
#define AUTH_UID_1      0x31
#define AUTH_UID_2      0xAE
#define AUTH_UID_3      0x02

#define CARD_COOLDOWN_MS    3000     // ignore same card for 3s
#define LCD_RESULT_MS       2000     // show result for 2s
```

To authorize a new card: tap it once (UID prints to UART), then update the `AUTH_UID_*` defines and reflash. To add runtime whitelist support, use the `add_uid` CLI extension point in `task_cli.c`.

---

## Key Technical Decisions

**Why bare-metal instead of HAL?**
HAL abstracts away the hardware. Direct register access forces you to understand SPI frame sizing bugs (DS bits), I2C TIMINGR math, and flash double-word alignment — exactly the knowledge that matters in production firmware work.

**Why initialize hardware in main() before the scheduler?**
Peripheral init functions like `LCD_Init()` use `vTaskDelay()` internally. Calling them from task context after the scheduler starts creates a race condition where the first tick fires before initialization completes. Pre-scheduler init with busy-wait delays eliminates this entirely.

**Why separate queues for keychar and keypad?**
The keypad task needs to send two different signals: individual keypresses (for live LCD `*` display) and complete PIN strings (for auth validation). Using two queues with different item sizes keeps the data paths clean and avoids polling inside the LCD task.

---

## Skills Demonstrated

- FreeRTOS task creation, preemptive scheduling, inter-task queues
- Bare-metal STM32 peripheral configuration (SPI, I2C, USART, RTC, Flash, GPIO)
- MFRC522 ISO14443A RFID protocol (REQA, anti-collision, UID read)
- HD44780 LCD in 4-bit mode via I2C PCF8574 expander
- Non-volatile storage: flash page erase, 64-bit double-word write, wear-level scanning
- RTC configuration with LSI, BCD register encoding, write-protection sequence
- IWDG watchdog configuration and reset-cause detection
- UART CLI design: character buffering, command dispatch, formatted output
- Embedded C best practices: volatile, const correctness, register-level bit manipulation

---

## Author

**Bhaumik Patel**
M.S. Electrical and Computer Engineering — San Francisco State University
[LinkedIn](https://linkedin.com/in/your-profile) | patelbhaumik226@gmail.com
