//!
//! @file
//! @author Marek Bel

#include "motion.h"
#include "stepper.h"
#include "permanent_storage.h"
#include <Arduino.h>

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
float _speed = 4500;
const uint16_t steps = BowdenLength::get();

    for (uint16_t i = 0; i < steps; i++)
    {
        do_pulley_step();

        if (i > 10 && i < 4000 && _speed > 650) _speed = _speed - 4;
        if (i > 100 && i < 4000 && _speed > 650) _speed = _speed - 1;
        if (i > 8000 && _speed < 3000) _speed = _speed + 2;
        delayMicroseconds(_speed);
    }
}

