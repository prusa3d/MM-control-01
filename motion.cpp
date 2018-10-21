#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "mmctl.h"
#include "Buttons.h"
#include "permanent_storage.h"
#include "stepper.h"

const int selector_steps_after_homing = -3700;
const int idler_steps_after_homing = -130;

const int selector_steps = 2790/4;
const int idler_steps = 1420 / 4;    // 2 msteps = 180 / 4
const int idler_parking_steps = (idler_steps / 2) + 40;  // 40
// endstop to tube  - 30 mm, 550 steps

// Accelerated stepper control
// (step_port, step_bit, dir_pin, TMC chip select)
Stepper pulley_stepper(&PORTB, 0x10, 1, AX_PUL, true);
Stepper selector_stepper(&PORTD, 0x10, 2, AX_SEL, false);
Stepper idler_stepper(&PORTD, 0x40, 4, AX_IDL, false);

// Coordinated stepper control
MultiStepper steppers;

int selector_steps_for_eject = 0;
int idler_steps_for_eject = 0;

int8_t filament_type[EXTRUDERS] = {-1, -1, -1, -1, -1};

void cut_filament();

void park_idler(bool _unpark);

void load_filament_inPrinter();
void load_filament_withSensor();

bool checkOk();

void cut_filament()
{
}

void set_positions(int _current_extruder, int _next_extruder)
{
	// steps to move to new position of idler and selector
	int _selector_steps = ((_current_extruder - _next_extruder) * selector_steps) * -1;
	int _idler_steps = (_current_extruder - _next_extruder) * idler_steps;

	// move both to new position
	idler_stepper.move(_idler_steps);
	selector_stepper.move(_selector_steps);
	steppers.wait();
}

void eject_filament(int extruder)
{
	//move selector sideways and push filament forward little bit, so user can catch it, unpark idler at the end to user can pull filament out
	int selector_position = 0;


	int8_t selector_offset_for_eject = 0;
	int8_t idler_offset_for_eject = 0;
		
	//if there is still filament detected by PINDA unload it first
	if (isFilamentLoaded)  unload_filament_withSensor();
	
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	
	tmc2130_init_axis(AX_PUL, tmc2130_mode);
		
	//if we are want to eject fil 0-2, move seelctor to position 4 (right), if we want to eject filament 3 - 4, move selector to position 0 (left)
	//maybe we can also move selector to service position in the future?
	if (extruder <= 2) selector_position = 4;
	else selector_position = 0;

	//count offset (number of positions) for desired selector and idler position for ejecting
	selector_offset_for_eject = active_extruder - selector_position;
	idler_offset_for_eject = active_extruder - extruder;

	//count number of desired steps for selector and idler and store it in static variable
	selector_steps_for_eject = (selector_offset_for_eject * selector_steps) * -1;
	idler_steps_for_eject = idler_offset_for_eject * idler_steps;

	//move selector and idler to new position
	idler_stepper.move(idler_steps_for_eject);
	selector_stepper.move(selector_steps_for_eject);
	steppers.wait();

	//push filament forward
	pulley_stepper.wait_move(2500);

	//unpark idler so user can easily remove filament
	park_idler(false);
	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void recover_after_eject()
{
	//restore state before eject filament
	tmc2130_init_axis(AX_PUL, tmc2130_mode);
	idler_stepper.move(-idler_steps_for_eject);
	selector_stepper.move(-selector_steps_for_eject);
	steppers.wait();
	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void load_filament_withSensor()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	tmc2130_init_axis(AX_PUL, tmc2130_mode);

	int _endstop_hit = 0;

	// load filament until FINDA senses end of the filament, means correctly loaded into the selector
	// we can expect something like 570 steps to get in sensor
	pulley_stepper.move(1500);
	while (digitalRead(A1) == 0 && pulley_stepper.run()) {}
	// Do not do a wait_stop() here, the next move(bowden_length) will set
	// the destination at exactly the point where the FINDA probe triggered.
	// If the FINDA probe did not trigger, we have already decelerated

	// If filament did not arrive at FINDA, let's try to correct that
	if (digitalRead(A1) == 0)
	{
		for (int i = 6; i > 0; i--)
		{
			if (digitalRead(A1) == 0)
			{
				// attempt to correct
				pulley_stepper.wait_move(-200);

				pulley_stepper.move(500);
				while (_endstop_hit<100 && pulley_stepper.one_step())
				{
					if (digitalRead(A1) == 1) _endstop_hit++;
				}
				pulley_stepper.wait_stop();
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

					pulley_stepper.wait_move(200);

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
		pulley_stepper.move(1500);
		while (digitalRead(A1) == 0 && pulley_stepper.one_step()) {}
		pulley_stepper.wait_stop();
		// ?
	}
	else
	{
		// nothing
	}

	pulley_stepper.wait_move(BowdenLength::get());

	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
	isFilamentLoaded = true;  // filament loaded 
}

void unload_filament_withSensor()
{
	// unloads filament from extruder - filament is above Bondtech gears
	tmc2130_init_axis(AX_PUL, tmc2130_mode);

	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	int _endstop_hit = 0;

	// unload until FINDA senses end of the filament
	pulley_stepper.move(-(long)BowdenLength::get() - 1000);

	while(_endstop_hit < 100 && pulley_stepper.one_step())
	{
		if (digitalRead(A1) == 0 && pulley_stepper.distanceToGo() > -2500)
			_endstop_hit++;
	}

	// FINDA is still sensing filament, let's try to unload it once again
	if (digitalRead(A1) == 1)
	{
		for (int i = 6; i > 0; i--)
		{
			if (digitalRead(A1) == 1)
			{
				pulley_stepper.wait_move(150);

				pulley_stepper.move(-4000);
				while (_endstop_hit < 100 && pulley_stepper.one_step())
					if (digitalRead(A1) == 0)
						_endstop_hit++;
				// Deaccelerate to a stop
				pulley_stepper.wait_stop();
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
				idler_stepper.wait_move(-200);
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
		// In a correct unload we are still moving at max velocity
		// and the FINDA probe has cleared 100 steps ago

		// Set destination into PTFE tube and decelerate to a stop
		pulley_stepper.wait_move(-450);
	}
	park_idler(false);
	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
	isFilamentLoaded = false; // filament unloaded 
}

void load_filament_inPrinter()
{
	// loads filament after confirmed by printer into the Bontech pulley gears so they can grab them
	uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
	uint8_t current_running_stealth[3] = CURRENT_RUNNING_STEALTH;
	uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
	uint8_t current_holding_stealth[3] = CURRENT_HOLDING_STEALTH;

	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	//PLA
	tmc2130_init_axis(AX_PUL, tmc2130_mode);
	pulley_stepper.setMaxSpeed(380); // 380 steps per second
	pulley_stepper.move(320);
	while (pulley_stepper.one_step())
	{
		if (pulley_stepper.distanceToGo() == 320 - 150)
		{ 
			if(tmc2130_mode == NORMAL_MODE)	
				tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL], current_running_normal[AX_PUL] - (current_running_normal[AX_PUL] / 4) ); 
			else 
				tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL], current_running_stealth[AX_PUL] - (current_running_stealth[AX_PUL] / 4) );
		}
	}

	//PLA
	if(tmc2130_mode == NORMAL_MODE)	
		tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL], current_running_normal[AX_PUL]/4);
	else 
		tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL], current_running_stealth[AX_PUL]/4);

	pulley_stepper.wait_move(450);
	pulley_stepper.setMaxSpeed(PULLEY_MAX_SPEED); // Reset max speed
	park_idler(false);
	tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void init_steppers()
{
	// Set max speed and max accelerations
	selector_stepper.setMaxSpeed(SELECTOR_MAX_SPEED);
	selector_stepper.setAcceleration(SELECTOR_MAX_ACCEL);

	pulley_stepper.setMaxSpeed(PULLEY_MAX_SPEED);
	pulley_stepper.setAcceleration(PULLEY_MAX_ACCEL);

	idler_stepper.setMaxSpeed(IDLER_MAX_SPEED);
	idler_stepper.setAcceleration(IDLER_MAX_ACCEL);

	// Coordinated movement manager
	steppers.addStepper(idler_stepper);
	steppers.addStepper(selector_stepper);
	steppers.addStepper(pulley_stepper);
}

void init_Pulley()
{
	// Blink LEDs while moving the pulley back and forth
	int i = 0;
	const int steps = 50;
	pulley_stepper.move(steps);
	while (pulley_stepper.one_step())
		shr16_set_led(1 << 2 * (int)(i++/steps));

	pulley_stepper.move(-steps);
	while (pulley_stepper.one_step())
		shr16_set_led(1 << 2 * (4-(int)(i++ / steps)));
}

void park_idler(bool _unpark)
{

	if (_unpark) // get idler in contact with filament
	{
		idler_stepper.wait_move(idler_parking_steps);
		isIdlerParked = false;
	} 
	else // park idler so filament can move freely
	{
		idler_stepper.wait_move(-idler_parking_steps);
		isIdlerParked = true;
	}
	 
}

bool home_idler(int sg_thr_neg, int sg_thr_pos)
{
	// Home idler drum
	// Return true if successful

	tmc2130_set_sg_thr(AX_IDL, sg_thr_neg);

	idler_stepper.setMaxSpeed(IDLER_HOMING_SPEED);
	int stalled = 0;

	idler_stepper.move(-IDLER_DRUM_STEP_LENGTH - 1000);
	while (idler_stepper.one_step()) {
		if (idler_stepper.is_stalled())
			if (++stalled > 16) {
				idler_stepper.setCurrentPosition(0);
				break;
			}
	}

	tmc2130_set_sg_thr(AX_IDL, sg_thr_pos);
	idler_stepper.move(IDLER_DRUM_STEP_LENGTH + 1000);
	uint16_t i = 0;
	while (idler_stepper.one_step()) {
		// Ignore stallguard for the first 200 steps
		if (i++ > 200 &&
			idler_stepper.is_stalled())
			if (++stalled > 16)
				break;
	}
	// Did we break out of the while loop?
	bool success = idler_stepper.distanceToGo() > 0;

	// Check that we were able to move the full length
	if (idler_stepper.distanceToGo() > 1000)
		return false;

	// Reset everything without using deacceleration
	idler_stepper.setCurrentPosition(0);
	idler_stepper.setMaxSpeed(IDLER_MAX_SPEED);
	return success;
}

bool home_selector()
{
	// Home selector
	// Return true if successful
	uint8_t stalled;

	selector_stepper.setMaxSpeed(SELECTOR_HOMING_SPEED);

	// Find the first limit
	selector_stepper.move(-SELECTOR_TRAY_LENGTH - 1000);
	stalled = 0;
	while (selector_stepper.one_step()) {
		if (selector_stepper.is_stalled()) {
			stalled++;
			if (stalled > 20) {
				// If we stalled reset everything without using deacceleration
				selector_stepper.setCurrentPosition(0);
				break;
			}
		}
	}

	// Find the other limit
	selector_stepper.move(SELECTOR_TRAY_LENGTH + 1000);
	stalled = 0;
	while (selector_stepper.one_step()) {
		if (selector_stepper.is_stalled()) {
			stalled++;
			if (stalled > 20)
				break;
		}
	}

	// Check that we were able to move the full length of the selector tray
	if (selector_stepper.distanceToGo() > 1000)
		return false;

	// Did we break out of the while loop?
	bool success = selector_stepper.distanceToGo() > 0;

	// Reset everything without using deacceleration
	selector_stepper.setCurrentPosition(0);
	selector_stepper.setMaxSpeed(SELECTOR_MAX_SPEED);
	return success;
}

void home()
{
	// Home idler and selector. Use different stallguard sensitivity levels
	// for idler
	isHomed = (
			home_idler(8, 7) ||
			home_idler(9, 8) ||
			home_idler(10, 9) ||
			home_idler(11, 10)
			) && home_selector();

	if (!isHomed) {
		while (true) {
			// Stop and warn about homing failure
			shr16_set_led(0x000);
			_delay_ms(200);
			shr16_set_led(0xFFF);
			_delay_ms(200);
		}
	}
	shr16_set_led(0x155);

	// We are homed, reset positions to zero
	idler_stepper.setCurrentPosition(0);
	pulley_stepper.setCurrentPosition(0);
	selector_stepper.setCurrentPosition(0);

	idler_stepper.move(idler_steps_after_homing);
	selector_stepper.move(selector_steps_after_homing);
	steppers.wait();

	active_extruder = 0;
	
	park_idler(false);

	shr16_set_led(0x000);
	
	isFilamentLoaded = false; 
	shr16_set_led(1 << 2 * (4-active_extruder));
}

void move(int _idler, int _selector, int _pulley)
{
	idler_stepper.move(_idler);
	selector_stepper.move(_selector);
	pulley_stepper.move(_pulley);
	steppers.wait();
}

bool checkOk()
{
	bool _ret = false;
	int _endstop_hit = 0;

	// filament in FINDA, let's try to unload it
	if (digitalRead(A1) == 1)
	{
		pulley_stepper.move(-3000);
		_endstop_hit = 0;
		while (pulley_stepper.one_step() > 0 && _endstop_hit < 50)
			if (digitalRead(A1) == 0) _endstop_hit++;
	}
	pulley_stepper.wait_stop();

	if (digitalRead(A1) == 0)
	{
		// looks ok, load filament to FINDA
		pulley_stepper.move(3000);
		_endstop_hit = 0;
		while(pulley_stepper.one_step() && _endstop_hit < 50)
			if (digitalRead(A1) == 1) _endstop_hit++;

		if (pulley_stepper.distanceToGo() == 0)
		{
			// we ran out of steps, means something is again wrong, abort
			_ret = false;
		}
		else
		{
			// looks ok !
			// unload to PTFE tube
			pulley_stepper.wait_move(-600);
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
