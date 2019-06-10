// main configuration file

#ifndef CONFIG_H_
#define CONFIG_H_

//timer0
//#define TIMER0_EVERY_1ms    _every_1ms    //1ms callback
//#define TIMER0_EVERY_10ms   _every_10ms   //10ms callback
//#define TIMER0_EVERY_100ms  _every_100ms  //100ms callback

//#define green_board

//shr16 - 16bit shift register
//pinout - hardcoded
//#define SHR16_CLK //signal d13 - PC7
//#define SHR16_LAT //signal d10 - PB6
//#define SHR16_DAT //signal d9  - PB5

//shift register outputs
//LEDS - hardcoded
//#define SHR16_LEDG0          0x0100
//#define SHR16_LEDR0          0x0200
//#define SHR16_LEDG1          0x0400
//#define SHR16_LEDR1          0x0800
//#define SHR16_LEDG2          0x1000
//#define SHR16_LEDR2          0x2000
//#define SHR16_LEDG3          0x4000
//#define SHR16_LEDR3          0x8000
//#define SHR16_LEDG4          0x0040
//#define SHR16_LEDR4          0x0080
#define SHR16_LED_MSK          0xffc0

//TMC2130 Direction/Enable signals - hardcoded
#define SHR16_DIR_0          0x0001
#define SHR16_ENA_0          0x0002
#define SHR16_DIR_1          0x0004
#define SHR16_ENA_1          0x0008
#define SHR16_DIR_2          0x0010
#define SHR16_ENA_2          0x0020
#define SHR16_DIR_MSK        (SHR16_DIR_0 + SHR16_DIR_1 + SHR16_DIR_2)
#define SHR16_ENA_MSK        (SHR16_ENA_0 + SHR16_ENA_1 + SHR16_ENA_2)

//UART0
#define UART0_BDR 115200

//UART1
#define UART1_BDR 115200

//stdin & stdout uart0/1
#define UART_STD 0
//communication uart0/1
#define UART_COM 1

//TMC2130 - Trinamic stepper driver
//pinout - hardcoded
//spi:
#define TMC2130_SPI_RATE       0 // fosc/4 = 4MHz
#define TMC2130_SPCR           SPI_SPCR(TMC2130_SPI_RATE, 1, 1, 1, 0)
#define TMC2130_SPSR           SPI_SPSR(TMC2130_SPI_RATE)
//params:
// SG_THR stallguard treshold (sensitivity), range -128..127, real 0-3
#define TMC2130_SG_THR_0       5
#define TMC2130_SG_THR_1       6
#define TMC2130_SG_THR_2       10
// TCOOLTHRS coolstep treshold, usable range 400-600
#define TMC2130_TCOOLTHRS_0    450
#define TMC2130_TCOOLTHRS_1    450
#define TMC2130_TCOOLTHRS_2    450

//0 - PULLEY
//1 - SELECTOR
//2 - IDLER
#define AX_PUL 0
#define AX_SEL 1
#define AX_IDL 2

// currents (R 0.22) 
// registers are from 0 to 31 only
// 0=1/32 31=32/32 from 0.96A with vsense=0 0.53A with vsense=1
// if current running <=31 vsense=1 
// if its more then its rigth shifted 1 bit
//           000 001 002 003 004 005 006 007 008 009 010 011 012 013 014 015 016 017 018 019 020 021 022 023 024 025 026 027 028 029 030 031
// vsense=1  017 033 050 066 083 099 116 133 149 166 182 199 215 232 248 265 282 298 315 331 348 364 381 398 414 431 447 464 480 497 513 530
// vsense=0  030 060 090 120 150 180 210 240 270 300 330 360 390 420 450 480 510 540 570 600 630 660 690 720 750 780 810 840 870 900 930 960
//
//           032 033 034 035 036 037 038 039 040 041 042 043 044 045 046 047 048 049 050 051 052 053 054 055 056 057 058 059 060 061 062 063
//           510 510 540 540 570 570 600 600 630 630 660 660 690 690 720 720 750 750 780 780 810 810 840 840 870 870 900 900 930 930 960 960    
#define CURRENT_HOLDING_STEALTH {1, 7, 22}  // {60,240,690 mA}
#define CURRENT_HOLDING_NORMAL {1, 10, 22}  // {17,330,690 mA}
#define CURRENT_RUNNING_STEALTH {35, 35, 45} // {540,540,690 mA}
#define CURRENT_RUNNING_NORMAL {30, 35, 47} // {513,540,720 mA}
#define CURRENT_HOMING {1, 35, 30} //

//mode
#define HOMING_MODE 0
#define NORMAL_MODE 1
#define STEALTH_MODE 2

//ADC configuration
#define ADC_CHAN_MSK      0b0000000000100000 //used AD channels bit mask (ADC5)
#define ADC_CHAN_CNT      1          //number of used channels)
#define ADC_OVRSAMPL      1          //oversampling multiplier
//#define ADC_READY         _adc_ready //ready callback


//signals (from interrupts to main loop)
#define SIG_ID_BTN             1 // any button changed

//states (<0 =error)
#define STA_INIT               0 //setup - initialization
#define STA_BUSY               1 //working
#define STA_READY              2 //ready - accepting commands

#define STA_ERR_TMC0_SPI      -1 //TMC2130 axis0 spi error - not responding 
#define STA_ERR_TMC0_MSC      -2 //TMC2130 axis0 motor error - short circuit
#define STA_ERR_TMC0_MOC      -3 //TMC2130 axis0 motor error - open circuit
#define STA_ERR_TMC0_PIN_STP  -4 //TMC2130 axis0 pin wirring error - stp signal
#define STA_ERR_TMC0_PIN_DIR  -5 //TMC2130 axis0 pin wirring error - dir signal
#define STA_ERR_TMC0_PIN_ENA  -6 //TMC2130 axis0 pin wirring error - ena signal

#define STA_ERR_TMC1_SPI      -11 //TMC2130 axis1 spi error - not responding 
#define STA_ERR_TMC1_MSC      -12 //TMC2130 axis1 motor error - short circuit
#define STA_ERR_TMC1_MOC      -13 //TMC2130 axis1 motor error - open circuit
#define STA_ERR_TMC1_PIN_STP  -14 //TMC2130 axis1 pin wirring error - stp signal
#define STA_ERR_TMC1_PIN_DIR  -15 //TMC2130 axis1 pin wirring error - dir signal
#define STA_ERR_TMC1_PIN_ENA  -16 //TMC2130 axis1 pin wirring error - ena signal

#define STA_ERR_TMC2_SPI      -21 //TMC2130 axis2 spi error - not responding 
#define STA_ERR_TMC2_MSC      -22 //TMC2130 axis2 motor error - short circuit
#define STA_ERR_TMC2_MOC      -23 //TMC2130 axis2 motor error - open circuit
#define STA_ERR_TMC2_PIN_STP  -24 //TMC2130 axis2 pin wirring error - stp signal
#define STA_ERR_TMC2_PIN_DIR  -25 //TMC2130 axis2 pin wirring error - dir signal
#define STA_ERR_TMC2_PIN_ENA  -26 //TMC2130 axis2 pin wirring error - ena signal

//number of extruders
#define EXTRUDERS 5

//diagnostic functions
//#define _DIAG

#endif //CONFIG_H_
