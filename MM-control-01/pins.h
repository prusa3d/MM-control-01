// @author Marek Bel

#ifndef PINS_H_
#define PINS_H_

#include <stdint.h>

const uint8_t selector_step_pin = 0x10;

inline void selector_step_pin_init()
{
    DDRD |= selector_step_pin;
}
inline void selector_step_pin_set()
{
    PORTD |= selector_step_pin;
}
inline void selector_step_pin_reset()
{
    PORTD &= ~selector_step_pin;
}

const uint8_t idler_step_pin = 0x40;

inline void idler_step_pin_init()
{
    DDRD |= idler_step_pin;
}
inline void idler_step_pin_set()
{
    PORTD |= idler_step_pin;
}
inline void idler_step_pin_reset()
{
    PORTD &= ~idler_step_pin;
}

const uint8_t pulley_step_pin = 0x10;

inline void pulley_step_pin_init()
{
    DDRB |= pulley_step_pin;
}
inline void pulley_step_pin_set()
{
    PORTB |= pulley_step_pin;
}
inline void pulley_step_pin_reset()
{
    PORTB &= ~pulley_step_pin;
}




#endif /* PINS_H_ */
