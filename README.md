# SmartDoor RTOS — Production-Grade Access Control Firmware

Bare-metal embedded firmware for the STM32L476RG (ARM Cortex-M4) implementing a multi-factor access control system with FreeRTOS preemptive scheduling, RTC-timestamped audit logging to internal flash, and a UART command-line interface — written entirely at the register level without STM32 HAL.

## What It Does

SmartDoor is a dual-authentication door access controller. A user can gain entry by either tapping an authorized RFID card or entering a 4-digit PIN on a matrix keypad. Every access attempt is permanently logged to internal flash with a real-time timestamp and can be retrieved over serial at any time.

## Hardware

| Component | Part | Interface |
|---|---|---|
| Microcontroller | STM32L476RG (ARM Cortex-M4) | — |
| RFID reader | MFRC522 | SPI1 |
| Keypad | 4x4 matrix | GPIO |
| Display | HD44780 16x2 LCD via PCF8574 | I2C1 |
| Debug/CLI | UART terminal | USART2 9600 baud |

## Features

- FreeRTOS with 5 concurrent tasks communicating via queues
- Bare-metal register-level drivers — no STM32 HAL
- RFID card + PIN dual authentication
- RTC-timestamped audit log written to internal flash
- UART CLI: dump_log, clear_log, set_time, status
- IWDG watchdog driver implemented

## Author

Bhaumik Patel — M.S. ECE, San Francisco State University
patelbhaumik226@gmail.com
