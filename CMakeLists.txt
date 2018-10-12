cmake_minimum_required(VERSION 3.0)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_C_COMPILER avr-gcc)
SET(CMAKE_CXX_COMPILER avr-g++)

SET(CSTANDARD "-std=gnu11")
SET(CPPSTANDARD "-std=gnu++11")

SET(WARN "-Wall -Wextra")
SET(CPPTUNING "-fno-exceptions -fno-threadsafe-statics")
SET(OPT "-Os -ffunction-sections -fdata-sections -flto -fno-fat-lto-objects")
SET(MCU "-mmcu=atmega32u4")

add_compile_definitions(
	F_CPU=16000000
	ARDUINO=10805
	ARDUINO_AVR_prusa_mm_control
	ARDUINO_ARCH_AVR
	USB_VID=0x2c99
	USB_PID=0x0004
	USB_MANUFACTURER="Unknown"
	USB_PRODUCT="Original Prusa i3 MK3 Multi Material 2.0 upgrade"
)

SET(CFLAGS "${OPT} ${WARN} ${CSTANDARD} ${MCU}")
SET(CXXFLAGS "${OPT} ${WARN} ${CPPSTANDARD} ${CPPTUNING} ${MCU}")
SET(ASMFLAGS "${OPT} ${WARN} ${MCU}")

SET(CMAKE_C_FLAGS ${CFLAGS})
SET(CMAKE_CXX_FLAGS ${CXXFLAGS})
SET(CMAKE_ASM_FLAGS ${ASMFLAGS})


project(MM-control-01 C CXX ASM)

set(SOURCES 
	MM-control-01/abtn3.c
	MM-control-01/adc.c
	MM-control-01/motion.cpp
	MM-control-01/main.cpp
	MM-control-01/tmc2130.c
	MM-control-01/permanent_storage.cpp
	MM-control-01/Buttons.cpp
	MM-control-01/uart.cpp
	MM-control-01/shr16.c
	MM-control-01/mmctl.cpp
	MM-control-01/spi.c
	core/abi.cpp
	core/hooks.c
	core/Stream.cpp
 	core/wiring_digital.c
	core/IPAddress.cpp
	core/Tone.cpp
	core/wiring_pulse.c
	core/CDC.cpp
	core/main.cpp
	core/wiring_pulse.S
	core/HardwareSerial0.cpp
	core/new.cpp
	core/USBCore.cpp
	core/wiring_shift.c
	core/HardwareSerial1.cpp
	core/WMath.cpp
	core/HardwareSerial2.cpp
	core/PluggableUSB.cpp
	core/WString.cpp
	core/HardwareSerial3.cpp
	core/HardwareSerial.cpp
	core/WInterrupts.c
	core/Print.cpp
	core/wiring_analog.c
	core/wiring.c	
)
add_executable(MM-control-01 ${SOURCES})
target_include_directories(MM-control-01 PRIVATE board core)

