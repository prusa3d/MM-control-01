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
#include "stepper.h"
#include "Buttons.h"
#include "permanent_storage.h"
#include "motion.h"

int active_extruder = 0;
int previous_extruder = -1;
bool isFilamentLoaded = false;
bool isPrinting = false;

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

		if (digitalRead(A1) == 1) { _loaded = true; _feed = false; };
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

	shr16_set_led(2 << 2 * (4 - active_extruder));

	previous_extruder = active_extruder;
	active_extruder = new_extruder;

    if (isFilamentLoaded && ((previous_extruder != active_extruder) || !isHomed()))
    {
        unload_filament_withSensor();
    }

    home();

    motion_set_idler_selector(active_extruder);

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
	home();

	shr16_set_led(2 << 2 * (4 - active_extruder));

	int previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder != active_extruder)
	{
        park_idler(true);
        motion_set_idler_selector((new_extruder < EXTRUDERS) ? new_extruder : (EXTRUDERS - 1) , new_extruder);
        park_idler(false);
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}

void eject_filament(int extruder)
{
    //move selector sideways and push filament forward little bit, so user can catch it, unpark idler at the end to user can pull filament out
    int selector_position = 0;
    int steps = 0;



    //if there is still filament detected by PINDA unload it first
    if (isFilamentLoaded)  unload_filament_withSensor();

    select_extruder(active_extruder); //Enforce home idler and selector.

    park_idler(true);

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    //if we are want to eject fil 0-2, move selector to position 4 (right), if we want to eject filament 3 - 4, move selector to position 0 (left)
    //maybe we can also move selector to service position in the future?
    if (extruder <= 2) selector_position = 4;
    else selector_position = 0;

    motion_set_idler_selector(extruder, selector_position);

    //push filament forward
    do
    {
        do_pulley_step();
        steps++;
        delayMicroseconds(1500);
    } while (steps < 2500);

    //unpark idler so user can easily remove filament
    park_idler(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void recover_after_eject()
{
    //restore state before eject filament
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    motion_set_idler_selector(active_extruder);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

bool checkOk()
{
    bool _ret = false;
    int _steps = 0;
    int _endstop_hit = 0;


    // filament in FINDA, let's try to unload it
    set_pulley_dir_pull();
    if (digitalRead(A1) == 1)
    {
        _steps = 3000;
        _endstop_hit = 0;
        do
        {
            do_pulley_step();
            delayMicroseconds(3000);
            if (digitalRead(A1) == 0) _endstop_hit++;
            _steps--;
        } while (_steps > 0 && _endstop_hit < 50);
    }

    if (digitalRead(A1) == 0)
    {
        // looks ok, load filament to FINDA
        set_pulley_dir_push();

        _steps = 3000;
        _endstop_hit = 0;
        do
        {
            do_pulley_step();
            delayMicroseconds(3000);
            if (digitalRead(A1) == 1) _endstop_hit++;
            _steps--;
        } while (_steps > 0 && _endstop_hit < 50);

        if (_steps == 0)
        {
            // we ran out of steps, means something is again wrong, abort
            _ret = false;
        }
        else
        {
            // looks ok !
            // unload to PTFE tube
            set_pulley_dir_pull();
            for (int i = 600; i > 0; i--)   // 570
            {
                do_pulley_step();
                delayMicroseconds(3000);
            }
            _ret = true;
        }

    }
    else
    {
        // something is wrong, abort
        _ret = false;
    }

    return _ret;
}

void load_filament_withSensor()
{
    FilamentLoaded::set(active_extruder);
    park_idler(true); // if idler is in parked position un-park him get in contact with filament

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    set_pulley_dir_push();

    int _loadSteps = 0;
    int _endstop_hit = 0;

    // load filament until FINDA senses end of the filament, means correctly loaded into the selector
    // we can expect something like 570 steps to get in sensor
    do
    {
        do_pulley_step();
        _loadSteps++;
        delayMicroseconds(5500);
    } while (digitalRead(A1) == 0 && _loadSteps < 1500);


    // filament did not arrived at FINDA, let's try to correct that
    if (digitalRead(A1) == 0)
    {
        for (int i = 6; i > 0; i--)
        {
            if (digitalRead(A1) == 0)
            {
                // attempt to correct
                set_pulley_dir_pull();
                for (int i = 200; i >= 0; i--)
                {
                    do_pulley_step();
                    delayMicroseconds(1500);
                }

                set_pulley_dir_push();
                _loadSteps = 0;
                do
                {
                    do_pulley_step();
                    _loadSteps++;
                    delayMicroseconds(4000);
                    if (digitalRead(A1) == 1) _endstop_hit++;
                } while (_endstop_hit<100 && _loadSteps < 500);
            }
        }
    }

    // still not at FINDA, error on loading, let's wait for user input
    if (digitalRead(A1) == 0)
    {
        bool _continue = false;
        bool _isOk = false;



        park_idler(false);
        do
        {
            shr16_set_led(0x000);
            delay(800);
            if (!_isOk)
            {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            }
            else
            {
                shr16_set_led(1 << 2 * (4 - active_extruder));
                delay(100);
                shr16_set_led(2 << 2 * (4 - active_extruder));
                delay(100);
            }
            delay(800);

            switch (buttonClicked())
            {
                case Btn::left:
                    // just move filament little bit
                    park_idler(true);
                    set_pulley_dir_push();

                    for (int i = 0; i < 200; i++)
                    {
                        do_pulley_step();
                        delayMicroseconds(5500);
                    }
                    park_idler(false);
                    break;
                case Btn::middle:
                    // check if everything is ok
                    park_idler(true);
                    _isOk = checkOk();
                    park_idler(false);
                    break;
                case Btn::right:
                    // continue with loading
                    park_idler(true);
                    _isOk = checkOk();
                    park_idler(false);

                    if (_isOk) //pridat do podminky flag ze od tiskarny prislo continue
                    {
                        _continue = true;
                    }
                    break;
                default:
                    break;
            }

        } while ( !_continue );






        park_idler(true);
        // TODO: do not repeat same code, try to do it until succesfull load
        _loadSteps = 0;
        do
        {
            do_pulley_step();
            _loadSteps++;
            delayMicroseconds(5500);
        } while (digitalRead(A1) == 0 && _loadSteps < 1500);
        // ?
    }
    else
    {
        // nothing
    }

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

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    isFilamentLoaded = true;  // filament loaded
}

void unload_filament_withSensor()
{
    // unloads filament from extruder - filament is above Bondtech gears
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    park_idler(true); // if idler is in parked position un-park him get in contact with filament

    set_pulley_dir_pull();

    float _speed = 2000;
    const float _first_point = 1800;
    const float _second_point = 8700;
    int _endstop_hit = 0;


    // unload until FINDA senses end of the filament
    int _unloadSteps = 10000;
    do
    {
        do_pulley_step();
        _unloadSteps--;

        if (_unloadSteps < 1400 && _speed < 6000) _speed = _speed + 3;
        if (_unloadSteps < _first_point && _speed < 2500) _speed = _speed + 2;
        if (_unloadSteps < _second_point && _unloadSteps > 5000 && _speed > 550) _speed = _speed - 2;

        delayMicroseconds(_speed);
        if (digitalRead(A1) == 0) _endstop_hit++;

    } while (_endstop_hit < 100 && _unloadSteps > 0);

    // move a little bit so it is not a grinded hole in filament
    for (int i = 100; i > 0; i--)
    {
        do_pulley_step();
        delayMicroseconds(5000);
    }



    // FINDA is still sensing filament, let's try to unload it once again
    if (digitalRead(A1) == 1)
    {
        for (int i = 6; i > 0; i--)
        {
            if (digitalRead(A1) == 1)
            {
                set_pulley_dir_push();
                for (int i = 150; i > 0; i--)
                {
                    do_pulley_step();
                    delayMicroseconds(4000);
                }

                set_pulley_dir_pull();
                int _steps = 4000;
                _endstop_hit = 0;
                do
                {
                    do_pulley_step();
                    _steps--;
                    delayMicroseconds(3000);
                    if (digitalRead(A1) == 0) _endstop_hit++;
                } while (_endstop_hit < 100 && _steps > 0);
            }
            delay(100);
        }

    }



    // error, wait for user input
    if (digitalRead(A1) == 1)
    {
        bool _continue = false;
        bool _isOk = false;

        park_idler(false);
        do
        {
            shr16_set_led(0x000);
            delay(100);
            if (!_isOk)
            {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            }
            else
            {
                shr16_set_led(1 << 2 * (4 - active_extruder));
                delay(100);
                shr16_set_led(2 << 2 * (4 - active_extruder));
                delay(100);
            }
            delay(100);


            switch (buttonClicked())
            {
            case Btn::left:
                // just move filament little bit
                park_idler(true);
                set_pulley_dir_pull();

                for (int i = 0; i < 200; i++)
                {
                    do_pulley_step();
                    delayMicroseconds(5500);
                }
                park_idler(false);
                break;
            case Btn::middle:
                // check if everything is ok
                park_idler(true);
                _isOk = checkOk();
                park_idler(false);
                break;
            case Btn::right:
                // continue with unloading
                park_idler(true);
                _isOk = checkOk();
                park_idler(false);

                if (_isOk)
                {
                    _continue = true;
                }
                break;
            default:
                break;
            }


        } while (!_continue);

        shr16_set_led(1 << 2 * (4 - previous_extruder));
        park_idler(true);
    }
    else
    {
        // correct unloading
        _speed = 5000;
        // unload to PTFE tube
        set_pulley_dir_pull();
        for (int i = 450; i > 0; i--)   // 570
        {
            do_pulley_step();
            delayMicroseconds(_speed);
        }
    }
    park_idler(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    isFilamentLoaded = false; // filament unloaded
}

//! @brief Do 320 pulley steps slower and 450 steps faster with decreasing motor current.
//!
//! @n d = 6.3 mm        pulley diameter
//! @n c = pi * d        pulley circumference
//! @n FSPR = 200        full steps per revolution (stepper motor constant) (1.8 deg/step)
//! @n mres = 2          microstep resolution (uint8_t __res(AX_PUL))
//! @n SPR = FSPR * mres steps per revolution
//! @n T1 = 2600 us      step period first segment
//! @n T2 = 2200 us      step period second segment
//! @n v1 = (1 / T1) / SPR * c = 19.02 mm/s  speed first segment
//! @n s1 =   320    / SPR * c = 15.80 mm    distance first segment
//! @n v2 = (1 / T2) / SPR * c = 22.48 mm/s  speed second segment
//! @n s2 =   450    / SPR * c = 22.26 mm    distance second segment
void load_filament_inPrinter()
{
    // loads filament after confirmed by printer into the Bontech pulley gears so they can grab them
    uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
    uint8_t current_running_stealth[3] = CURRENT_RUNNING_STEALTH;
    uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
    uint8_t current_holding_stealth[3] = CURRENT_HOLDING_STEALTH;

    park_idler(true); // if idler is in parked position un-park him get in contact with filament
    set_pulley_dir_push();

    //PLA
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    for (int i = 0; i <= 320; i++)
    {
        if (i == 150)
        {
            if(tmc2130_mode == NORMAL_MODE)
                tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL], current_running_normal[AX_PUL] - (current_running_normal[AX_PUL] / 4) );
            else
                tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL], current_running_stealth[AX_PUL] - (current_running_stealth[AX_PUL] / 4) );
        }
        do_pulley_step();
        delayMicroseconds(2600);
    }

    //PLA
    if(tmc2130_mode == NORMAL_MODE)
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL], current_running_normal[AX_PUL]/4);
    else
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL], current_running_stealth[AX_PUL]/4);

    for (int i = 0; i <= 450; i++)
    {
        do_pulley_step();
        delayMicroseconds(2200);
    }

    park_idler(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}
