/*
 * MIT License
 *
 * Copyright (c) 2018 Erriez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*!
 * \file ErriezTM1637.h
 * \brief TM1637 library for Arduino
 * \details
 *      Source:         https://github.com/Erriez/ErriezTM1637
 *      Documentation:  https://erriez.github.io/ErriezTM1637
 * \verbatim
   Command / register definitions

   MSB           LSB
    7 6 5 4 3 2 1 0
   -----------------
    0 1 - - - - - -    Data command
    1 0 - - - - - -    Display control command
    1 1 - - - - - -    Address command


   7.1 Data Command Set

   MSB           LSB
    7 6 5 4 3 2 1 0
   -----------------
    0 1 0 0 0 - 0 0    Write display data
    0 1 0 0 0 - 1 0    Read key scan data
    0 1 0 0 0 0 - -    Auto address increment
    0 1 0 0 0 1 - -    Fixed address


   7.2 Address command set

   MSB           LSB
    7 6 5 4 3 2 1 0
   -----------------
    1 1 0 - A A A A    Address 0x00..0x0F


   7.3 Display Control

   MSB           LSB
    7 6 5 4 3 2 1 0
   -----------------
    1 0 0 0 - 0 0 0    Set the pulse width of 1 / 16
    1 0 0 0 - 0 0 1    Set the pulse width of 2 / 16
    1 0 0 0 - 0 1 0    Set the pulse width of 4 / 16
    1 0 0 0 - 0 1 1    Set the pulse width of 10 / 16
    1 0 0 0 - 1 0 0    Set the pulse width of 11 / 16
    1 0 0 0 - 1 0 1    Set the pulse width of 12 / 16
    1 0 0 0 - 1 1 0    Set the pulse width of 13 / 16
    1 0 0 0 - 1 1 1    Set the pulse width of 14 / 16
    1 0 0 0 0 - - -    Display off
    1 0 0 0 1 - - -    Display on
   \endverbatim
 */

#ifndef ERRIEZ_TM1637_H_
#define ERRIEZ_TM1637_H_

#include <Arduino.h>
//#undef FIX_THIS

#if 0
#ifdef __AVR
#define FIX_THIS
#endif 
#endif
// Commands
#define TM1637_CMD_DATA                 0x40 //!< Display data command
#define TM1637_CMD_CTRL                 0x80 //!< Display control command
#define TM1637_CMD_ADDR                 0xc0 //!< Display address command

// Data command bits
#define TM1637_DATA_WRITE               0x00 //!< Write data
#define TM1637_DATA_READ_KEYS           0x02 //!< Read keys
#define TM1637_DATA_AUTO_INC_ADDR       0x00 //!< Auto increment address
#define TM1637_DATA_FIXED_ADDR          0x04 //!< Fixed address

// Control command bits
#define TM1637_CTRL_PULSE_1_16          0x00 //!< Pulse width 1/16
#define TM1637_CTRL_PULSE_2_16          0x01 //!< Pulse width 2/16
#define TM1637_CTRL_PULSE_4_16          0x02 //!< Pulse width 4/16
#define TM1637_CTRL_PULSE_10_16         0x03 //!< Pulse width 10/16
#define TM1637_CTRL_PULSE_11_16         0x04 //!< Pulse width 11/16
#define TM1637_CTRL_PULSE_12_16         0x05 //!< Pulse width 12/16
#define TM1637_CTRL_PULSE_13_16         0x06 //!< Pulse width 13/16
#define TM1637_CTRL_PULSE_14_16         0x07 //!< Pulse width 14/16
#define TM1637_CTRL_DISPLAY_OFF         0x00 //!< Display off
#define TM1637_CTRL_DISPLAY_ON          0x08 //!< Display on

#define TM1637_NUM_GRIDS                6    //!< Number of grid registers

#ifdef FIX_THIS
#define TM1637_CLK_LOW()        { *portOutputRegister(_clkPort) &= ~_clkBit; }  //!< CLK pin low
#define TM1637_CLK_HIGH()       { *portOutputRegister(_clkPort) |= _clkBit; }   //!< CLK pin high
#define TM1637_CLK_INPUT()      { *portModeRegister(_clkPort) &= ~_clkBit; }    //!< CLK pin input
#define TM1637_CLK_OUTPUT()     { *portModeRegister(_clkPort) |= _clkBit; }     //!< CLK pin output
#define TM1637_DIO_LOW()        { *portOutputRegister(_dioPort) &= ~_dioBit; }  //!< DIO pin low
#define TM1637_DIO_HIGH()       { *portOutputRegister(_dioPort) |= _dioBit; }   //!< DIO pin high
#define TM1637_DIO_INPUT()      { *portModeRegister(_dioPort) &= ~_dioBit; }    //!< DIO pin input
#define TM1637_DIO_OUTPUT()     { *portModeRegister(_dioPort) |= _dioBit; }     //!< DIO pin output
#define TM1637_DIO_READ()       ( *portInputRegister(_dioPort) & _dioBit )      //!< DIO pin read
#else
#define TM1637_CLK_LOW()        { digitalWrite(_clkPin, LOW); }                 //!< CLK pin low
#define TM1637_CLK_HIGH()       { digitalWrite(_clkPin, HIGH); }                //!< CLK pin high
#define TM1637_CLK_INPUT()      { pinMode(_clkPin, INPUT); }                    //!< CLK pin input
#define TM1637_CLK_OUTPUT()     { pinMode(_clkPin, OUTPUT); }                   //!< CLK pin output
#define TM1637_DIO_LOW()        { digitalWrite(_dioPin, LOW); }                 //!< DIO pin low
#define TM1637_DIO_HIGH()       { digitalWrite(_dioPin, HIGH); }                //!< DIO pin high
#define TM1637_DIO_INPUT()      { pinMode(_dioPin, INPUT); }                    //!< DIO pin input
#define TM1637_DIO_OUTPUT()     { pinMode(_dioPin, OUTPUT); }                   //!< DIO pin output
#define TM1637_DIO_READ()       ( digitalRead(_dioPin) )                        //!< DIO pin read
#endif

#if F_CPU >= 20000000UL
#define TM1637_PIN_DELAY()      { delayMicroseconds(1); }           //!< Delay between pin changes
#else
#define TM1637_PIN_DELAY()     {  asm("nop");    }                               //!< Delay between pin changes
#endif


/*!
 * \brief TM1637 class
 */
class TM1637
{
public:
    TM1637(uint8_t clkPin, uint8_t dioPin, bool displayOn=true, uint8_t brightness=5);

    virtual void begin();
    virtual void end();
    virtual void displayOn();
    virtual void displayOff();
    virtual void setBrightness(uint8_t brightness);
    virtual void clear();
    virtual void writeData(uint8_t address, uint8_t data);
    virtual void writeData(uint8_t address, const uint8_t *buf, uint8_t len);
    virtual uint8_t getKeys();

protected:
#ifdef FIX_THIS
    uint8_t _clkPort;   //!< Clock port in IO pin register
    uint8_t _dioPort;   //!< Data port in IO pin register

    uint8_t _clkBit;    //!< Clock bit number in IO pin register
    uint8_t _dioBit;    //!< Data bit number in IO pin register

#else
    uint8_t _clkPin;    //!< Clock pin
    uint8_t _dioPin;    //!< Data pin
#endif

    bool    _displayOn;  //!< Display on and off status for display control register
    uint8_t _brightness; //!< Display brightness for display control register

    virtual void writeDisplayControl();
    virtual void writeCommand(uint8_t cmd);
    virtual void start();
    virtual void stop();
    virtual void writeByte(uint8_t data);
    virtual uint8_t readByte();
};

#endif // ERRIEZ_TM1637_H_
