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
static bool s_selector_homed = false;
static bool s_idler_engaged = true;
static bool s_has_door_sensor = false;

void rehome()
{
    s_idler = 0;
    s_selector = 0;
    shr16_set_ena(0);
    delay(10);
    shr16_set_ena(7);
    tmc2130_init(tmc2130_mode);
    home();
    if (s_idler_engaged) park_idler(true);
}

static void rehome_idler()
{
    shr16_set_ena(0);
    delay(10);
    shr16_set_ena(7);
    tmc2130_init(tmc2130_mode);
    home_idler();
    int idler_steps = get_idler_steps(0, s_idler);
    move_proportional(idler_steps, 0);
    if (s_idler_engaged) park_idler(true);
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
//! @param idler idler
//! @param selector selector
void motion_set_idler_selector(uint8_t idler, uint8_t selector)
{
    if (!s_selector_homed)
    {
            home();
            s_selector = 0;
            s_idler = 0;
            s_selector_homed = true;
    }
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
            if (tries == i) unrecoverable_error();
            drive_error();
            rehome();
        }
    }
}

static void check_idler_drive_error()
{
    const uint8_t tries = 2;
    for (uint8_t i = 0; i <= tries; ++i)
    {
        if (!tmc2130_read_gstat()) break;
        else
        {
            if (tries == i) unrecoverable_error();
            drive_error();
            rehome_idler();
        }
    }
}

void motion_engage_idler()
{
    s_idler_engaged = true;
    park_idler(true);
    check_idler_drive_error();
}

void motion_disengage_idler()
{
    s_idler_engaged = false;
    park_idler(false);
    check_idler_drive_error();
}

//! @brief unload until FINDA senses end of the filament
static void unload_to_finda()
{
    int delay = 2000; //microstep period in microseconds
    const int _first_point = 1800;

    uint8_t _endstop_hit = 0;

    int _unloadSteps = BowdenLength::get() + 1100;
    const int _second_point = _unloadSteps - 1300;

    set_pulley_dir_pull();

    while (_endstop_hit < 100u && _unloadSteps > 0)
    {
        do_pulley_step();
        _unloadSteps--;

        if (_unloadSteps < 1400 && delay < 6000) delay += 3;
        if (_unloadSteps < _first_point && delay < 2500) delay += 2;
        if (_unloadSteps < _second_point && _unloadSteps > 5000)
        {
            if (delay > 550) delay -= 1;
            if (delay > 330 && (NORMAL_MODE == tmc2130_mode)) delay -= 1;
        }

        delayMicroseconds(delay);
        if (digitalRead(A1) == 0) _endstop_hit++;

    }
}

void motion_feed_to_bondtech()
{
    int stepPeriod = 4500; //microstep period in microseconds
    const uint16_t steps = BowdenLength::get();

    const uint8_t tries = 2;
    for (uint8_t tr = 0; tr <= tries; ++tr)
    {
        set_pulley_dir_push();
        unsigned long delay = 4500;

        for (uint16_t i = 0; i < steps; i++)
        {
            delayMicroseconds(delay);
            unsigned long now = micros();

            if (i < 4000)
            {
                if (stepPeriod > 2600) stepPeriod -= 4;
                if (stepPeriod > 1300) stepPeriod -= 2;
                if (stepPeriod > 650) stepPeriod -= 1;
                if (stepPeriod > 350 && (NORMAL_MODE == tmc2130_mode) && s_has_door_sensor) stepPeriod -= 1;
            }
            if (i > (steps - 800) && stepPeriod < 2600) stepPeriod += 10;
            if ('A' == getc(uart_com))
            {
                s_has_door_sensor = true;
                tmc2130_disable_axis(AX_PUL, tmc2130_mode);
                motion_disengage_idler();
                return;
            }
            do_pulley_step();
            delay = stepPeriod - (micros() - now);
        }

        if (!tmc2130_read_gstat()) break;
        else
        {
            if (tries == tr) unrecoverable_error();
            drive_error();
            rehome_idler();
            unload_to_finda();
        }
    }
}




//! @brief unload to FINDA
//!
//! Check for drive error and try to recover 3 times.
void motion_unload_to_finda()
{
    const uint8_t tries = 2;
    for (uint8_t tr = 0; tr <= tries; ++tr)
    {
        unload_to_finda();
        if (tmc2130_read_gstat() && digitalRead(A1) == 1)
        {
            if (tries == tr) unrecoverable_error();
            drive_error();
            rehome_idler();
        }
        else
        {
            break;
        }
    }

}

void motion_door_sensor_detected()
{
    s_has_door_sensor = true;
}

void motion_set_idler(uint8_t idler)
{
    home_idler();
    int idler_steps = get_idler_steps(0, idler);
    move_proportional(idler_steps, 0);
    s_idler = idler;
}
