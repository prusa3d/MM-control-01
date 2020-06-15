//! @file
//! @brief High level multimaterial switcher control

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "stepper.h"
#include "Buttons.h"
#include "motion.h"
#include "permanent_storage.h"
#include "config.h"

//! Keeps track of selected filament. It is used for LED signalization and it is backed up to permanent storage
//! so MMU can unload filament after power loss.
int active_extruder = 0;
//! Keeps track of filament crossing selector. Selector can not be moved if filament crosses it.
bool isFilamentLoaded = false;

//! Number of pulley steps to eject and un-eject filament
static const int eject_steps = 2500;

//! @brief Change filament
//!
//! Unload filament, if different filament than requested is currently loaded,
//! or homing wasn't done yet.
//! Home if not homed.
//! Switch to requested filament (this does nothing if requested filament is currently selected).
//! Load filament if not loaded.
//! @param new_extruder Filament to be selected
void switch_extruder_withSensor(int new_extruder)
{
	shr16_set_led(2 << 2 * (4 - active_extruder));

	active_extruder = new_extruder;

    if (isFilamentLoaded)
    {
        unload_filament_withoutSensor();
    }

    motion_set_idler2(active_extruder);

    shr16_set_led(2 << 2 * (4 - active_extruder));

    if (!isFilamentLoaded)
    {
            load_filament_withoutSensor();
    }

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}

//! @brief Select filament
//!
//! Does not unload or load filament, just moves selector and idler,
//! caller is responsible for ensuring, that filament is not loaded when caled.
//!
//! @param new_extruder Filament to be selected
void select_extruder(int new_extruder)
{
	shr16_set_led(2 << 2 * (4 - active_extruder));

	active_extruder = new_extruder;

    motion_set_idler2((new_extruder < EXTRUDERS) ? new_extruder : (EXTRUDERS - 1));

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}


//! @brief eject filament
//! Move selector sideways and push filament forward little bit, so user can catch it,
//! unpark idler at the end to user can pull filament out.
//! If there is still filament detected by PINDA unload it first.
//! If we are want to eject fil 0-2, move selector to position 4 (right),
//! if we want to eject filament 3 - 4, move selector to position 0 (left)
//! maybe we can also move selector to service position in the future?
//! @param filament filament 0 to 4
void eject_filament(uint8_t filament)
{
    active_extruder = filament;

    if (isFilamentLoaded)  unload_filament_withoutSensor();

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    motion_set_idler2(filament);

    motion_engage_idler();
    set_pulley_dir_push();

    for (int steps = 0; steps < eject_steps; ++steps)
    {
        do_pulley_step();
        steps++;
        delayMicroseconds(1500);
    }

    motion_disengage_idler();
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

//! @brief restore state before eject filament
void recover_after_eject()
{
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    motion_engage_idler();
    set_pulley_dir_pull();
    for (int steps = 0; steps < eject_steps; ++steps)
    {
        do_pulley_step();
        steps++;
        delayMicroseconds(1500);
    }
    motion_disengage_idler();

    motion_set_idler2(active_extruder);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

//! @brief Load filament through bowden
//! @param disengageIdler
//!  * true Disengage idler after movement
//!  * false Do not disengage idler after movement
void load_filament_withoutSensor(bool disengageIdler)
{
    FilamentLoaded::set(active_extruder);
    motion_engage_idler();

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    set_pulley_dir_push();
    pinda_state = 1;
    motion_feed_to_bondtech();

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    if (disengageIdler) motion_disengage_idler();
    isFilamentLoaded = true;  // filament loaded
}

void unload_filament_withoutSensor()
{
    // unloads filament from extruder - filament is above Bondtech gears
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    motion_engage_idler(); // if idler is in parked position un-park him get in contact with filament

    motion_unload_to_finda();
    pinda_state = 0;
    motion_disengage_idler();
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    isFilamentLoaded = false; // filament unloaded
}

//! @brief Do 38.20 mm pulley push
//!
//! Load filament after confirmed by printer into the Bontech pulley gears so they can grab them.
//! Stop when 'A' received
//!
//! @n d = 6.3 mm        pulley diameter
//! @n c = pi * d        pulley circumference
//! @n FSPR = 200        full steps per revolution (stepper motor constant) (1.8 deg/step)
//! @n mres = 2          microstep resolution (uint8_t __res(AX_PUL))
//! @n SPR = FSPR * mres steps per revolution
//! @n T1 = 2600 us      step period first segment
//! @n v1 = (1 / T1) / SPR * c = 19.02 mm/s  speed first segment
//! @n s1 =   770    / SPR * c = 38.10 mm    distance first segment
void load_filament_inPrinter()
{

    motion_engage_idler();
    set_pulley_dir_push();

    const unsigned long fist_segment_delay = 2600;

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    unsigned long delay = fist_segment_delay;

    for (int i = 0; i < 770; i++)
    {
        delayMicroseconds(delay);
        unsigned long now = micros();

        if ('A' == getc(uart_com))
        {
            motion_door_sensor_detected();
            break;
        }
        do_pulley_step();
        delay = fist_segment_delay - (micros() - now);
    }

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    motion_disengage_idler();
}
