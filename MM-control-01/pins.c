#include <avr/io.h>
#include "pins.h"

const uint8_t selector_step_pin = 0x10;
const uint8_t idler_step_pin = 0x40;
const uint8_t pulley_step_pin = 0x10;

extern inline void selector_step_pin_init();
extern inline void selector_step_pin_set();
extern inline void selector_step_pin_reset();
extern inline void idler_step_pin_init();
extern inline void idler_step_pin_set();
extern inline void idler_step_pin_reset();
extern inline void pulley_step_pin_init();
extern inline void pulley_step_pin_set();
extern inline void pulley_step_pin_reset();
