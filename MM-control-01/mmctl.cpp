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

//! @brief Feed filament to FINDA
//!
//! Continuously feed filament until FINDA is not switched ON
//! and than retracts to align filament 600 steps away from FINDA.
//! @param timeout
//!  * true feed phase is limited, doesn't react on button press
//!  * false feed phase is unlimited, can be interrupted by any button press after blanking time
//! @retval true Selector is aligned on FINDA, FINDA was switched ON
//! @retval false Selector is not probably aligned on FINDA ,FINDA was not switched ON
bool feed_filament(bool timeout)
{
	bool loaded = false;
	const uint_least8_t finda_limit = 10;

	motion_engage_idler();
	set_pulley_dir_push();
	if(tmc2130_mode == NORMAL_MODE)
	{
		tmc2130_init_axis_current_normal(AX_PUL, 1, 15);
	}
	else
	{
		tmc2130_init_axis_current_stealth(AX_PUL, 1, 15); //probably needs tuning of currents
	}

	{
	    uint_least8_t blinker = 0;
	    uint_least8_t button_blanking = 0;
	    const uint_least8_t button_blanking_limit = 11;
	    uint_least8_t finda_triggers = 0;

        for (unsigned int steps = 0; !timeout || (steps < 1500); ++steps)
        {
            do_pulley_step();
            ++blinker;

            if (blinker > 50)
            {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            }
            if (blinker > 100)
            {
                shr16_set_led(0x000);
                blinker = 0;
                if (button_blanking <= button_blanking_limit) ++button_blanking;
            }

            if (digitalRead(A1) == 1) ++finda_triggers;
            if (finda_triggers >= finda_limit)
            {
                loaded = true;
                break;
            }
            if (!timeout && (buttonPressed() != Btn::none) && (button_blanking >= button_blanking_limit))
            {
                break;
            }
            delayMicroseconds(4000);
        }
	}

	if (loaded)
	{
		// unload to PTFE tube
		set_pulley_dir_pull();
		for (int i = 600 + finda_limit; i > 0; i--)
		{
			do_pulley_step();
			delayMicroseconds(3000);
		}
	}

	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
	motion_disengage_idler();
	shr16_set_led(1 << 2 * (4 - active_extruder));

	return loaded;
}

//! @brief Try to resolve non-loaded filamnt to selector
//!
//! button | action
//! ------ | ------
//! middle | Try to rehome selector and align filament to FINDA if it success, blinking will stop
//! right  | If no red LED is blinking, resume print, else same as middle button 
//!
void resolve_failed_loading(){
    bool resolved = false;
    bool exit = false;
    while(!exit){
        switch (buttonClicked())
        {
            case Btn::middle:
                rehome();
                motion_set_idler_selector(active_extruder);
                if(feed_filament(true)){resolved = true;}
            break;

            case Btn::right:
                if(!resolved){
                    rehome();
                    motion_set_idler_selector(active_extruder);
                    if(feed_filament(true)){resolved = true;}
                }
                
                if(resolved){
                    motion_set_idler_selector(active_extruder);
                    motion_engage_idler();                    
                    exit = true;
                }
            break;

            default:
                if(resolved){
                    signal_ok_after_load_failure();}
                else{
                    signal_load_failure();}
                
            break;
        }
    }
}

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
        unload_filament_withSensor();
    }

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
	shr16_set_led(2 << 2 * (4 - active_extruder));

	active_extruder = new_extruder;

    motion_set_idler_selector((new_extruder < EXTRUDERS) ? new_extruder : (EXTRUDERS - 1) , new_extruder);

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}
//! @brief cut filament
//! @param filament filament 0 to 4
void mmctl_cut_filament(uint8_t filament)
{
    const int cut_steps_pre = 700;
    const int cut_steps_post = 150;

    active_extruder = filament;

    if (isFilamentLoaded)  unload_filament_withSensor();

    motion_set_idler_selector(filament, filament);

    if(!feed_filament(true)){resolve_failed_loading();}
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    motion_set_idler_selector(filament, filament + 1);

    motion_engage_idler();
    set_pulley_dir_push();

    for (int steps = 0; steps < cut_steps_pre; ++steps)
    {
        do_pulley_step();
        steps++;
        delayMicroseconds(1500);
    }
    motion_set_idler_selector(filament, 0);
    set_pulley_dir_pull();

    for (int steps = 0; steps < cut_steps_post; ++steps)
    {
        do_pulley_step();
        steps++;
        delayMicroseconds(1500);
    }
    motion_set_idler_selector(filament, 5);
    motion_set_idler_selector(filament, 0);
    motion_set_idler_selector(filament, filament);
    if(!feed_filament(true)){resolve_failed_loading();}
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
    const uint8_t selector_position = (filament <= 2) ? 4 : 0;

    if (isFilamentLoaded)  unload_filament_withSensor();

    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    motion_set_idler_selector(filament, selector_position);

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

    motion_set_idler_selector(active_extruder);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

static bool checkOk()
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

//! @brief Can FINDA detect filament tip
//!
//! Move filament back and forth to align it by FINDA.
//! @retval true success
//! @retval false failure
bool mmctl_IsOk()
{
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    motion_engage_idler();
    const bool retval = checkOk();
    motion_disengage_idler();
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    return retval;
}

//! @brief Load filament through bowden
//! @param disengageIdler
//!  * true Disengage idler after movement
//!  * false Do not disengage idler after movement
void load_filament_withSensor(bool disengageIdler)
{
    FilamentLoaded::set(active_extruder);
    motion_engage_idler();

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



        motion_disengage_idler();
        do
        {
            if (!_isOk)
            {
                signal_load_failure();
            }
            else
            {
                signal_ok_after_load_failure();
            }

            switch (buttonPressed())
            {
                case Btn::left:
                    // just move filament little bit
                    motion_engage_idler();
                    set_pulley_dir_push();

                    for (int i = 0; i < 200; i++)
                    {
                        do_pulley_step();
                        delayMicroseconds(5500);
                    }
                    motion_disengage_idler();
                    break;
                case Btn::middle:
                    // check if everything is ok
                    motion_engage_idler();
                    _isOk = checkOk();
                    motion_disengage_idler();
                    break;
                case Btn::right:
                    // continue with loading
                    motion_engage_idler();
                    _isOk = checkOk();
                    motion_disengage_idler();

                    if (_isOk) //pridat do podminky flag ze od tiskarny prislo continue
                    {
                        _continue = true;
                    }
                    break;
                default:
                    break;
            }

        } while ( !_continue );

        motion_engage_idler();
        set_pulley_dir_push();
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

    motion_feed_to_bondtech();

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    if (disengageIdler) motion_disengage_idler();
    isFilamentLoaded = true;  // filament loaded
}

void unload_filament_withSensor()
{
    // unloads filament from extruder - filament is above Bondtech gears
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    motion_engage_idler(); // if idler is in parked position un-park him get in contact with filament

    if (digitalRead(A1))
    {
        motion_unload_to_finda();
    }
    else
    {
        if (checkOk())
        {
            motion_disengage_idler();
            return;
        }
    }



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
                uint8_t _endstop_hit = 0;
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

        motion_disengage_idler();
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


            switch (buttonPressed())
            {
            case Btn::left:
                // just move filament little bit
                motion_engage_idler();
                set_pulley_dir_pull();

                for (int i = 0; i < 200; i++)
                {
                    do_pulley_step();
                    delayMicroseconds(5500);
                }
                motion_disengage_idler();
                break;
            case Btn::middle:
                // check if everything is ok
                motion_engage_idler();
                _isOk = checkOk();
                motion_disengage_idler();
                break;
            case Btn::right:
                // continue with unloading
                motion_engage_idler();
                _isOk = checkOk();
                motion_disengage_idler();

                if (_isOk)
                {
                    _continue = true;
                }
                break;
            default:
                break;
            }


        } while (!_continue);

        shr16_set_led(1 << 2 * (4 - active_extruder));
        motion_engage_idler();
    }
    else
    {
        // correct unloading
        // unload to PTFE tube
        set_pulley_dir_pull();
        for (int i = 450; i > 0; i--)   // 570
        {
            do_pulley_step();
            delayMicroseconds(5000);
        }
    }
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
