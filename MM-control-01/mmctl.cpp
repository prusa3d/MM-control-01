//mmctl.cpp - multimaterial switcher control
#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"

int active_extruder = 0;
int previous_extruder = -1;
bool isFilamentLoaded = false;
bool isIdlerParked = false;
int toolChanges = 0;

bool isPrinting = false;
bool isHomed = false;

bool feed_filament()
{
	bool _feed = true;
	bool _loaded = false;

	int _c = 0;
	int _delay = 0;
	park_idler(true);

	set_pulley_dir_push();
	if(tmc2130_mode == NORMAL_MODE)	
		tmc2130_init_axis_current_normal(AX_PUL, 1, 15);
	else
		tmc2130_init_axis_current_stealth(AX_PUL, 1, 15); //probably needs tuning of currents

	do
	{
		do_pulley_step();
		
		_c++;
		if (_c > 50) { shr16_set_led(2 << 2 * (4 - active_extruder)); };
		if (_c > 100) { shr16_set_led(0x000); _c = 0; _delay++; };

		if (READ(PIN_A1) == 1) { _loaded = true; _feed = false; };
		if (buttonClicked() != Btn::none && _delay > 10) { _loaded = false; _feed = false; }
		delayMicroseconds(4000);
	} while (_feed);

	if (_loaded)
	{
		// unload to PTFE tube
		set_pulley_dir_pull();
		for (int i = 600; i > 0; i--)   // 570
		{
			do_pulley_step();
			delayMicroseconds(3000);
		}
	}



	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
	park_idler(false);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return true;
}

//! @brief Change filament
//!
//! Unload filament, if different filament than requested is currently loaded,
//! or homing wasn't done yet.
//! Home if not homed.
//! Switch to requested filament (this does nothing if requested filament is currently selected).
//! Load filament if not loaded.
//! @par new_extruder Filament to be selected
void switch_extruder_withSensor(int new_extruder)
{
	isPrinting = true;
	
	if (active_extruder == 5)
	{
		move(0, -700, 0);
		active_extruder = 4;
	}
	
	toolChanges++;

	shr16_set_led(2 << 2 * (4 - active_extruder));

	previous_extruder = active_extruder;
	active_extruder = new_extruder;

    if (isFilamentLoaded && ((previous_extruder != active_extruder) || !isHomed))
    {
        unload_filament_withSensor();
    }

    if (!isHomed)
    {
        home();
    }

    set_positions(previous_extruder, active_extruder);

    shr16_set_led(2 << 2 * (4 - active_extruder));

    if (!isFilamentLoaded)
    {
            load_filament_withSensor();
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
	if (!isHomed) { home(); }

	shr16_set_led(2 << 2 * (4 - active_extruder));

	previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder != active_extruder)
	{

		if (new_extruder == 5)
		{
			move(0, 700, 0);
			++previous_extruder;
		}
	    else if (previous_extruder == 5)
        {
            move(0, -700, 0);
            previous_extruder = 4;
        }

        if (isIdlerParked) park_idler(true);
        set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
        park_idler(false);
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}

void led_blink(int _no)
{
	shr16_set_led(1 << 2 * _no);
	delay(40);
	shr16_set_led(0x000);
	delay(20);
	shr16_set_led(1 << 2 * _no);
	delay(40);

	shr16_set_led(0x000);
	delay(10);
}


