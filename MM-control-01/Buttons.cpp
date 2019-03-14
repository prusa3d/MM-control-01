//! @file

#include "Buttons.h"
#include "shr16.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "permanent_storage.h"
#include "main.h"



void settings_bowden_length();


//!	@brief Select filament for bowden length calibration
//!
//! Filaments are selected by left and right buttons, calibration is activated by middle button.
//! Park position (one behind last filament) can be also selected.
//! Activating calibration in park position exits selector.
//!
void settings_select_filament()
{
	while (1)
	{
		manual_extruder_selector();

		if(Btn::middle == buttonClicked())
		{
			shr16_set_led(2 << 2 * (4 - active_extruder));
			delay(500);
			if (Btn::middle == buttonClicked())
			{
			    if (!isHomed) { home(); }
				if (active_extruder < 5) settings_bowden_length();
				else
				{
					select_extruder(0);
					return;
				}
			}
		}
	}
}

//!	@brief Show setup menu
//!
//! Items are selected by left and right buttons, activated by middle button.
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 11 | 00 | 00 | 00 | 01 | initial state, no action
//! 11 | 00 | 00 | 01 | 00 | setup bowden length
//! 11 | 00 | 01 | 00 | 00 | erase EEPROM if unlocked
//! 11 | 01 | 00 | 00 | 00 | unlock EEPROM erase
//! 11 | 00 | 00 | 00 | 00 | exit setup menu
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//!
void setupMenu()
{
	shr16_set_led(0x000);
	delay(200);
	shr16_set_led(0x2aa);
	delay(1200);
	shr16_set_led(0x000);
	delay(600);

	int _menu = 0;
	bool _exit = false;
	bool eraseLocked = true;

		

	do
	{
		shr16_set_led(1 << 2 * 4);
		delay(1);
		shr16_set_led(2 << 2 * 4);
		delay(1);
		shr16_set_led(2 << 2 * _menu);
		delay(1);

		switch (buttonClicked())
		{
		case Btn::right:
			if (_menu > 0) { _menu--; delay(800); }
			break;
		case Btn::middle:
				
			switch (_menu)
			{
				case 1:
					settings_select_filament();
					_exit = true;
					break;
				case 2:
					if (!eraseLocked)
					{
						eepromEraseAll();
						_exit = true;
					}
					break;
				case 3: //unlock erase
					eraseLocked = false;
					break;
				case 4: // exit menu
					_exit = true;
					break;
			}
			break;
		case Btn::left:
			if (_menu < 4) { _menu++; delay(800); }
			break;
		default:
			break;
		}
		
	} while (!_exit);


	shr16_set_led(0x000);
	delay(400);
	shr16_set_led(0x2aa);
	delay(400);
	shr16_set_led(0x000);
	delay(400);

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}

//! @brief Set bowden length
//!
//! button | action
//! ------ | ------
//! left   | increase bowden length / feed more filament
//! right  | decrease bowden length / feed less filament
//! middle | store bowden length to EEPROM and exit
//!
//! This state is indicated by following LED pattern:
//!
//! RG | RG | RG | RG | RG
//! -- | -- | -- | -- | --
//! bb | 00 | 00 | 0b | 00
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
//!
void settings_bowden_length()
{
	// load filament above Bondtech gears to check correct length of bowden tube
	if (!isFilamentLoaded)
	{
		BowdenLength bowdenLength;
		load_filament_withSensor();

		tmc2130_init_axis_current_normal(AX_PUL, 1, 30);
		uint32_t saved_millis=millis();
		bool button_active = false;
		do
		{

			switch (buttonClicked())
			{
			case Btn::right:
				if (!button_active || (((millis() - saved_millis) > 1000) && button_active)) {
					if (bowdenLength.decrease()) {
						move(0, 0, -bowdenLength.stepSize);
					}
				}
				button_active = true;
				break;
			case Btn::left:
				if (!button_active || (((millis() - saved_millis) > 1000) && button_active)) {
					if(bowdenLength.increase()) {
						move(0, 0, bowdenLength.stepSize);
					}
				}
				button_active = true;
				break; 
			default:
				button_active = false;
				saved_millis = millis();
				break;
			}

			shr16_set_led(1 << 2 * 4);
			delay(10);
			shr16_set_led(2 << 2 * 4);
			delay(10);
			shr16_set_led(2 << 2 * 1);
			delay(50);


		} while (buttonClicked() != Btn::middle);

		unload_filament_withSensor();
	}
}

//! @brief Is button pushed?
//!
//! @return button pushed
// moved to shr16
/*

*/

