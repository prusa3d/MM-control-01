//!
//! @file
//! @author Marek Bel

#include "motion.h"
#include "stepper.h"
#include "permanent_storage.h"
#include <Arduino.h>
#include "main.h"

static uint8_t s_idler = 0;
static uint8_t s_selector = 0;

void motion_set_idler_selector(uint8_t idler_selector)
{
    motion_set_idler_selector(idler_selector, idler_selector);
}

void motion_set_idler_selector(uint8_t idler, uint8_t selector)
{
    home();
    int idler_steps = get_idler_steps(s_idler, idler);
    int selector_steps = get_selector_steps(s_selector, selector);

    move_proportional(idler_steps, selector_steps);
    s_idler = idler;
    s_selector = selector;
}

void motion_engage_idler()
{
    park_idler(true);
}

void motion_disengage_idler()
{
    park_idler(false);
}

void motion_feed_to_bondtech()
{
int _speed = 4500;
const uint16_t steps = BowdenLength::get();

    for (uint16_t i = 0; i < steps; i++)
    {
        do_pulley_step();
        delayMicroseconds(_speed);

        if (i < 4000)
        {
            if (_speed > 1600) _speed = _speed - 20;
            if (_speed > 800) _speed = _speed - 10;
            if (_speed > 400) _speed = _speed - 5;
            if (_speed > 200) _speed = _speed - 4;
            if (_speed > 150) _speed = _speed - 1;
        }
        if (i > (steps - 800) && _speed < 3000) _speed = _speed + 10;
        if ('A' == getc(uart_com))
        {
            return;
        }
    }
}

