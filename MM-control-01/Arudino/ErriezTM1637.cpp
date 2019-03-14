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
 * \file ErriezTM1637.cpp
 * \brief TM1637 library for Arduino
 * \details
 *      Source:         https://github.com/Erriez/ErriezTM1637
 *      Documentation:  https://erriez.github.io/ErriezTM1637
 */

#include "ErriezTM1637.h"

/*!
 * \brief TM1637 constructor
 * \details
 *    Constructor with pin arguments: C-D (Clock, Data)
 * \param clkPin
 *    TM1637 CLK pin.
 * \param dioPin
 *    TM1637 DIO pin.
 * \param displayOn
 *    true: Turn display on (default)
 *    false: Turn display off
 * \param brightness
 *    Display brightness value 0..7
 */
TM1637::TM1637(uint8_t clkPin, uint8_t dioPin, bool displayOn, uint8_t brightness) :
    _displayOn(displayOn), _brightness((uint8_t)(brightness & 0x07))
{
#ifdef FIX_THIS
    // Calculate bit and port register for fast pin read and writes (AVR targets only)
    _clkPort = digitalPinToPort(clkPin);
    _dioPort = digitalPinToPort(dioPin);

    _clkBit = digitalPinToBitMask(clkPin);
    _dioBit = digitalPinToBitMask(dioPin);
#else
    // Use the slow digitalRead() and digitalWrite() functions for non-AVR targets
    _dioPin = dioPin;
    _clkPin = clkPin;
#endif
}

/*!
 * \brief Initialize TM1637 controller.
 */
void TM1637::begin()
{
    // Make DIO and CLK pins high
    TM1637_DIO_HIGH();
    TM1637_CLK_HIGH();

    // Set pin mode
    TM1637_DIO_OUTPUT();
    TM1637_CLK_OUTPUT();

    // Write _displayOn and _brightness to display control register
    writeDisplayControl();

    // Data write with auto address increment
    writeCommand(TM1637_CMD_DATA | TM1637_DATA_WRITE | TM1637_DATA_AUTO_INC_ADDR);
}

/*!
 * \brief Release TM1637 pins.
 */
void TM1637::end()
{
    // Release DIO and CLK pins
    TM1637_DIO_INPUT();
    TM1637_CLK_INPUT();

    TM1637_DIO_LOW();
    TM1637_CLK_LOW();
}

/*!
 * \brief Turn Display on.
 */
void TM1637::displayOn()
{
    _displayOn = true;

    writeDisplayControl();
}

/*!
 * \brief Turn display off.
 */
void TM1637::displayOff()
{
    _displayOn = false;

    writeDisplayControl();
}

/*!
 * \brief Set brightness LED's.
 * \param brightness
 *    Display brightness value 0..7
 */
void TM1637::setBrightness(uint8_t brightness)
{
    if (brightness <= 7) {
        _brightness = brightness;

        writeDisplayControl();
    }
}

/*!
 * \brief Turn all LED's off.
 */
void TM1637::clear()
{
    start();
    writeByte((uint8_t)(TM1637_CMD_ADDR | 0x00));
    for (uint8_t i = 0; i < TM1637_NUM_GRIDS; i++) {
        writeByte(0x00);
    }
    stop();
}

/*!
 * \brief Write display register
 * \param address Display address 0x00..0x05
 * \param data Value 0x00..0xFF
 */
void TM1637::writeData(uint8_t address, uint8_t data)
{
    if (address < TM1637_NUM_GRIDS) {
        start();
        writeByte((uint8_t)(TM1637_CMD_ADDR | address));
        writeByte(data);
        stop();
    }
}

/*!
 * \brief Write buffer to multiple display registers
 * \details
 *    Write buffer to TM1638 with auto address increment
 * \param address
 *    Display address 0x00..0x05
 * \param buf
 *    Buffer
 * \param len
 *    Buffer length
 */
void TM1637::writeData(uint8_t address, const uint8_t *buf, uint8_t len)
{
    if ((address + len) <= TM1637_NUM_GRIDS) {
        // Write buffer to display registers
        start();
        writeByte((uint8_t)(TM1637_CMD_ADDR | address));
        for (uint8_t i = 0; i < len; i++) {
            writeByte(buf[i]);
        }
        stop();
    }
}

/*!
 * \brief Get key states.
 * \returns
 *      0xFF: All keys up
 *      0x00..0x0F: Key 0..15 down
 */
uint8_t TM1637::getKeys() 
{   
    uint8_t keyCode;
   
    start();
    writeByte(TM1637_CMD_DATA | TM1637_DATA_READ_KEYS);
    start();
    keyCode = readByte();
    stop();

    // Check if key is down (at least one bit is zero)
    if (keyCode != 0xFF) {
        // Invert keyCode:
        //    Bit |  7  6  5  4  3  2  1  0
        //  ------+-------------------------
        //   From | S0 S1 S2 K1 K2 1  1  1
        //     To | S0 S1 S2 K1 K2 0  0  0
        keyCode = ~keyCode;
        
        // Shift bits to: 
        //    Bit | 7  6  5  4  3  2  1  0
        //  ------+------------------------
        //     To | 0  0  0  0  K2 S2 S1 S0
        keyCode = (uint8_t)((keyCode & 0x80) >> 7 |
                            (keyCode & 0x40) >> 5 |
                            (keyCode & 0x20) >> 3 |
                            (keyCode & 0x08));
    }

    return keyCode;
}

// -------------------------------------------------------------------------------------------------
/*!
 * \brief Write display control register.
 */
void TM1637::writeDisplayControl()
{
    // Write to display control register
    start();
    writeByte((uint8_t)(TM1637_CMD_CTRL |
                        (_displayOn ? TM1637_CTRL_DISPLAY_ON : TM1637_CTRL_DISPLAY_OFF) |
                         _brightness));
    stop();
}

/*!
 * \brief Write command to TM1637.
 */
void TM1637::writeCommand(uint8_t cmd)
{
    start();
    writeByte(cmd);
    stop();
}

// -------------------------------------------------------------------------------------------------
/*! 
 * \brief Generate start condition
 */
void TM1637::start(void)
{
    TM1637_DIO_LOW();
    TM1637_PIN_DELAY();
    TM1637_CLK_LOW();
    TM1637_PIN_DELAY();
}

/*! 
 * \brief Generate stop condition
 */
void TM1637::stop(void)
{
    TM1637_DIO_LOW();
    TM1637_PIN_DELAY();
    TM1637_CLK_HIGH();
    TM1637_PIN_DELAY();
    TM1637_DIO_HIGH();
    TM1637_PIN_DELAY();
}

/*!
 * \brief Write byte to TM1637.
 * \param data 8-bit value.
 */
void TM1637::writeByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        TM1637_PIN_DELAY();
        TM1637_CLK_LOW();

        if (data & 0x01) {
            TM1637_DIO_HIGH();
        } else {
            TM1637_DIO_LOW();
        }
        data >>= 1;

        TM1637_PIN_DELAY();
        TM1637_CLK_HIGH();
    }

    TM1637_PIN_DELAY();
    TM1637_CLK_LOW();

    // Set DIO pin to input with pull-up to read ACK
    TM1637_DIO_HIGH();
    TM1637_DIO_INPUT();
    TM1637_PIN_DELAY();

    TM1637_CLK_HIGH();
    TM1637_PIN_DELAY();
    // Skip reading DIO pin: ack = _dioPin;
    TM1637_CLK_LOW();
    TM1637_PIN_DELAY();

    // Set DIO pin to output
    TM1637_DIO_OUTPUT();
    TM1637_PIN_DELAY();
}

/*!
 * \brief Read byte from TM1637.
 * \return 8-bit value.
 */
uint8_t TM1637::readByte()
{
    uint8_t retval = 0;
    
    // Prepare DIO to read data
    TM1637_DIO_HIGH();
    TM1637_DIO_INPUT();
    TM1637_PIN_DELAY();
    
    // Data is shifted out by the TM1637 on the CLK falling edge
    for (uint8_t bit = 0; bit < 8; bit++) {
        TM1637_CLK_HIGH();
        TM1637_PIN_DELAY();
        
        // Read next bit
        retval <<= 1;
        if (TM1637_DIO_READ()) { 
            retval |= 0x01; 
        }
        
        TM1637_CLK_LOW();
        TM1637_PIN_DELAY();
    }
    
    // Return DIO to output mode
    TM1637_DIO_OUTPUT();
    TM1637_PIN_DELAY();
    
    // Dummy ACK
    TM1637_DIO_LOW();
    TM1637_PIN_DELAY();
    
    TM1637_CLK_HIGH();
    TM1637_PIN_DELAY();
    TM1637_CLK_LOW();
    TM1637_PIN_DELAY();
    
    TM1637_DIO_HIGH();
    TM1637_PIN_DELAY();
    
    return retval;
}
