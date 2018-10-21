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
#include "stepper.h"

extern Stepper pulley_stepper;

int active_extruder = -1;
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

	if(tmc2130_mode == NORMAL_MODE)	
		tmc2130_init_axis_current_normal(AX_PUL, 1, 15);
	else
		tmc2130_init_axis_current_stealth(AX_PUL, 1, 15); //probably needs tuning of currents

	pulley_stepper.move(20000);
	do
	{
		pulley_stepper.one_step();
		
		_c++;
		if (_c > 50) { shr16_set_led(2 << 2 * (4 - active_extruder)); };
		if (_c > 100) { shr16_set_led(0x000); _c = 0; _delay++; };

		if (digitalRead(A1) == 1) {
			_loaded = true;
			_feed = false;
			// We want to unload 600 steps from here, so set the new destination
			pulley_stepper.move(-600);
		};
		if (buttonClicked() != Btn::none && _delay > 10) { _loaded = false; _feed = false; }
	} while (_feed);

	if (_loaded)
	{
		// unload to PTFE tube
		pulley_stepper.runToPosition();
	} else {
		// Stop as quickly as possible with deceleration
		pulley_stepper.wait_stop();
	}



	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
	park_idler(false);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return true;
}

bool switch_extruder_withSensor(int new_extruder)
{
	
	isPrinting = true;
	bool _return = false;
	if (!isHomed) { home(); }
	
	if (active_extruder == 5)
	{
		move(0, -700, 0);
		active_extruder = 4;
	}
	
	
	toolChanges++;

	shr16_set_led(2 << 2 * (4 - active_extruder));

	previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded)
		{
			shr16_set_led(2 << 2 * (4 - active_extruder));
			load_filament_withSensor(); // just load filament if not loaded
			_return = true;
		}
		else
		{
			_return = false;  // nothing really happened
		}
	}
	else
	{
		if (isFilamentLoaded) { unload_filament_withSensor(); } // unload filament first
		set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
		
		shr16_set_led(2 << 2 * (4 - active_extruder));
		load_filament_withSensor(); // load new filament
		_return = true;
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return _return;


}

//! @brief select extruder
//!
//! Known limitation is, that if extruder 5 - service position was selected before
//! it is not possible to select any other extruder than extruder 4.
//!
//! @param new_extruder Extruder to be selected
//! @return
bool select_extruder(int new_extruder)
{

	bool _return = false;
	if (!isHomed) { home(); }

	shr16_set_led(2 << 2 * (4 - active_extruder));

	int previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded)
		{
			_return = true;
		}
	}
	else
	{
		if (new_extruder == 5)
		{
			move(0, 700, 0);
		}
		else
		{
			if (previous_extruder == 5)
			{
				move(0, -700, 0);
			}
			else
			{
				if (isIdlerParked) park_idler(true);
				set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
				park_idler(false);
			}
		}
		_return = true;
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return _return;
}

bool service_position()
{
	move(0, 600, 0);

	return true;
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


