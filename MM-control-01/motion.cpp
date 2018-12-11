//!
//! @file
//! @author Marek Bel

#include "motion.h"
#include "stepper.h"
#include "permanent_storage.h"
#include <Arduino.h>
#include "main.h"
#include "config.h"

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
    set_pulley_dir_push();

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
            if ((_speed > 150) && (NORMAL_MODE == tmc2130_mode)) _speed = _speed - 1;
        }
        if (i > (steps - 800) && _speed < 3000) _speed = _speed + 10;
        if ('A' == getc(uart_com))
        {
            return;
        }
    }
}

//! @brief unload until FINDA senses end of the filament
void motion_unload_to_finda()
{
    int _speed = 2000;
    const int _first_point = 1800;

    uint8_t _endstop_hit = 0;

    int _unloadSteps = BowdenLength::get() + 1100;
    const int _second_point = _unloadSteps - 1300;

    set_pulley_dir_pull();

    while (_endstop_hit < 100u && _unloadSteps > 0)
    {
        do_pulley_step();
        _unloadSteps--;

        if (_unloadSteps < 1400 && _speed < 6000) _speed = _speed + 3;
        if (_unloadSteps < _first_point && _speed < 2500) _speed = _speed + 2;
        if (_unloadSteps < _second_point && _unloadSteps > 5000)
        {
            if (_speed > 550) _speed = _speed - 1;
            if (_speed > 250 && (NORMAL_MODE == tmc2130_mode)) _speed = _speed - 1;
        }

        delayMicroseconds(_speed);
        if (digitalRead(A1) == 0) _endstop_hit++;

    }
}
