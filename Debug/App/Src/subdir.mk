################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../App/Src/application.cpp \
../App/Src/common.cpp \
../App/Src/cppmain.cpp \
../App/Src/iicx.cpp \
../App/Src/log.cpp \
../App/Src/timx.cpp 

C_SRCS += \
../App/Src/syscallsx.c 

C_DEPS += \
./App/Src/syscallsx.d 

OBJS += \
./App/Src/application.o \
./App/Src/common.o \
./App/Src/cppmain.o \
./App/Src/iicx.o \
./App/Src/log.o \
./App/Src/syscallsx.o \
./App/Src/timx.o 

CPP_DEPS += \
./App/Src/application.d \
./App/Src/common.d \
./App/Src/cppmain.d \
./App/Src/iicx.d \
./App/Src/log.d \
./App/Src/timx.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.cpp App/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103x6 -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"M:/STM32CubeIDE/workspace/f103c6-car/App/Inc" -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.c App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103x6 -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"M:/STM32CubeIDE/workspace/f103c6-car/App/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-App-2f-Src

clean-App-2f-Src:
	-$(RM) ./App/Src/application.cyclo ./App/Src/application.d ./App/Src/application.o ./App/Src/application.su ./App/Src/common.cyclo ./App/Src/common.d ./App/Src/common.o ./App/Src/common.su ./App/Src/cppmain.cyclo ./App/Src/cppmain.d ./App/Src/cppmain.o ./App/Src/cppmain.su ./App/Src/iicx.cyclo ./App/Src/iicx.d ./App/Src/iicx.o ./App/Src/iicx.su ./App/Src/log.cyclo ./App/Src/log.d ./App/Src/log.o ./App/Src/log.su ./App/Src/syscallsx.cyclo ./App/Src/syscallsx.d ./App/Src/syscallsx.o ./App/Src/syscallsx.su ./App/Src/timx.cyclo ./App/Src/timx.d ./App/Src/timx.o ./App/Src/timx.su

.PHONY: clean-App-2f-Src

