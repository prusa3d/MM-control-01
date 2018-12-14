//!
//! @file
//! @author Marek Bel

#include "motion.h"
#include "stepper.h"
#include "permanent_storage.h"
#include <Arduino.h>
#include "main.h"
#include "config.h"
#include "tmc2130.h"
#include "shr16.h"

static uint8_t s_idler = 0;
static uint8_t s_selector = 0;
static bool s_idler_engaged = true;
static bool s_has_door_sensor = false;

static void rehome()
{
    s_idler = 0;
    s_selector = 0;
    shr16_set_ena(0);
    delay(10);
    shr16_set_ena(7);
    tmc2130_init(tmc2130_mode);
    home(true);
    if (!s_idler_engaged) motion_disengage_idler();
}

void motion_set_idler_selector(uint8_t idler_selector)
{
    motion_set_idler_selector(idler_selector, idler_selector);
}

//! @brief move idler and selector to desired location
//!
//! In case of drive error re-home and try to recover 3 times.
//! If the drive error is permanent call unrecoverable_error();
//!
//! @par idler idler
//! @par selector selector
void motion_set_idler_selector(uint8_t idler, uint8_t selector)
{
    home();
    const uint8_t tries = 2;
    for (uint8_t i = 0; i <= tries; ++i)
    {
        int idler_steps = get_idler_steps(s_idler, idler);
        int selector_steps = get_selector_steps(s_selector, selector);

        move_proportional(idler_steps, selector_steps);
        s_idler = idler;
        s_selector = selector;

        if (!tmc2130_read_gstat()) break;
        else
        {
            drive_error();
            rehome();
        }
        if (tries == i) unrecoverable_error();
    }
}

void motion_engage_idler()
{
    s_idler_engaged = true;
    park_idler(true);
}

void motion_disengage_idler()
{
    s_idler_engaged = false;
    park_idler(false);
}

void motion_feed_to_bondtech()
{
    int _speed = 4500;
    const uint16_t steps = BowdenLength::get();
    set_pulley_dir_push();
    unsigned long delay = 4500;

    for (uint16_t i = 0; i < steps; i++)
    {
        delayMicroseconds(delay);
        unsigned long now = micros();

        if (i < 4000)
        {
            if (_speed > 2600) _speed = _speed - 6;
            if (_speed > 1300) _speed = _speed - 3;
            if (_speed > 650) _speed = _speed - 2;
            if (_speed > 250 && (NORMAL_MODE == tmc2130_mode) && s_has_door_sensor) _speed = _speed - 1;
        }
        if (i > (steps - 800) && _speed < 2600) _speed = _speed + 10;
        if ('A' == getc(uart_com))
        {
            s_has_door_sensor = true;
            tmc2130_disable_axis(AX_PUL, tmc2130_mode);
            motion_disengage_idler();
            return;
        }
        do_pulley_step();
        delay = _speed - (micros() - now);
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

void motion_door_sensor_detected()
{
    s_has_door_sensor = true;
}
