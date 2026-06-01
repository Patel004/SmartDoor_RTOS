################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/Source/list.c \
../FreeRTOS/Source/queue.c \
../FreeRTOS/Source/stream_buffer.c \
../FreeRTOS/Source/tasks.c \
../FreeRTOS/Source/timers.c 

OBJS += \
./FreeRTOS/Source/list.o \
./FreeRTOS/Source/queue.o \
./FreeRTOS/Source/stream_buffer.o \
./FreeRTOS/Source/tasks.o \
./FreeRTOS/Source/timers.o 

C_DEPS += \
./FreeRTOS/Source/list.d \
./FreeRTOS/Source/queue.d \
./FreeRTOS/Source/stream_buffer.d \
./FreeRTOS/Source/tasks.d \
./FreeRTOS/Source/timers.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/%.o FreeRTOS/Source/%.su FreeRTOS/Source/%.cyclo: ../FreeRTOS/Source/%.c FreeRTOS/Source/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32L4 -DSTM32 -DSTM32L476RGTx -c -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/Inc" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/Src" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/CMSIS/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/CMSIS/Device/ST/STM32L4xx/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/FreeRTOS/Include" -I"/Users/bhaumik/Desktop/STM32Projects/SmartDoor_RTOS/FreeRTOS/portable/GCC/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-Source

clean-FreeRTOS-2f-Source:
	-$(RM) ./FreeRTOS/Source/list.cyclo ./FreeRTOS/Source/list.d ./FreeRTOS/Source/list.o ./FreeRTOS/Source/list.su ./FreeRTOS/Source/queue.cyclo ./FreeRTOS/Source/queue.d ./FreeRTOS/Source/queue.o ./FreeRTOS/Source/queue.su ./FreeRTOS/Source/stream_buffer.cyclo ./FreeRTOS/Source/stream_buffer.d ./FreeRTOS/Source/stream_buffer.o ./FreeRTOS/Source/stream_buffer.su ./FreeRTOS/Source/tasks.cyclo ./FreeRTOS/Source/tasks.d ./FreeRTOS/Source/tasks.o ./FreeRTOS/Source/tasks.su ./FreeRTOS/Source/timers.cyclo ./FreeRTOS/Source/timers.d ./FreeRTOS/Source/timers.o ./FreeRTOS/Source/timers.su

.PHONY: clean-FreeRTOS-2f-Source

