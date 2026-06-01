################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/RFID.c \
../Src/flash_log.c \
../Src/keypad.c \
../Src/lcd.c \
../Src/main.c \
../Src/rtc.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/task_auth.c \
../Src/task_cli.c \
../Src/task_keypad.c \
../Src/task_lcd.c \
../Src/task_rfid.c \
../Src/uart.c \
../Src/watchdog.c 

OBJS += \
./Src/RFID.o \
./Src/flash_log.o \
./Src/keypad.o \
./Src/lcd.o \
./Src/main.o \
./Src/rtc.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/task_auth.o \
./Src/task_cli.o \
./Src/task_keypad.o \
./Src/task_lcd.o \
./Src/task_rfid.o \
./Src/uart.o \
./Src/watchdog.o 

C_DEPS += \
./Src/RFID.d \
./Src/flash_log.d \
./Src/keypad.d \
./Src/lcd.d \
./Src/main.d \
./Src/rtc.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/task_auth.d \
./Src/task_cli.d \
./Src/task_keypad.d \
./Src/task_lcd.d \
./Src/task_rfid.d \
./Src/uart.d \
./Src/watchdog.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32L4 -DSTM32 -DSTM32L476RGTx -c -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/Inc" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/Src" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/CMSIS/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/CMSIS/Device/ST/STM32L4xx/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/FreeRTOS/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/FreeRTOS/portable/GCC/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/RFID.cyclo ./Src/RFID.d ./Src/RFID.o ./Src/RFID.su ./Src/flash_log.cyclo ./Src/flash_log.d ./Src/flash_log.o ./Src/flash_log.su ./Src/keypad.cyclo ./Src/keypad.d ./Src/keypad.o ./Src/keypad.su ./Src/lcd.cyclo ./Src/lcd.d ./Src/lcd.o ./Src/lcd.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/rtc.cyclo ./Src/rtc.d ./Src/rtc.o ./Src/rtc.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/task_auth.cyclo ./Src/task_auth.d ./Src/task_auth.o ./Src/task_auth.su ./Src/task_cli.cyclo ./Src/task_cli.d ./Src/task_cli.o ./Src/task_cli.su ./Src/task_keypad.cyclo ./Src/task_keypad.d ./Src/task_keypad.o ./Src/task_keypad.su ./Src/task_lcd.cyclo ./Src/task_lcd.d ./Src/task_lcd.o ./Src/task_lcd.su ./Src/task_rfid.cyclo ./Src/task_rfid.d ./Src/task_rfid.o ./Src/task_rfid.su ./Src/uart.cyclo ./Src/uart.d ./Src/uart.o ./Src/uart.su ./Src/watchdog.cyclo ./Src/watchdog.d ./Src/watchdog.o ./Src/watchdog.su

.PHONY: clean-Src

