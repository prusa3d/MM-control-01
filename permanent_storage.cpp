//! @file
//! @author Marek Bel

#include "permanent_storage.h"
#include "mmctl.h"
#include <avr/eeprom.h>



//! @brief Is filament number valid?
//! @retval true valid
//! @retval false invalid
bool validFilament(uint8_t filament)
{
	if (filament < (sizeof(eeprom_t::eepromBowdenLen)/sizeof(eeprom_t::eepromBowdenLen[0]))) return true;
	else return false;
}

//! @brief Is bowden length in valid range?
//! @param BowdenLength bowden length
//! @retval true valid
//! @retval false invalid
static bool validBowdenLen (const uint16_t BowdenLength)
{
	if ((BowdenLength >= eepromBowdenLenMinimum)
			&& BowdenLength <= eepromBowdenLenMaximum) return true;
	return false;
}

//! @brief Get bowden length for active filament
//!
//! Returns stored value, doesn't return actual value when it is edited by increase() / decrease() unless it is stored.
//! @return stored bowden length
uint16_t BowdenLength::get()
{
	uint8_t filament = active_extruder;
	if (validFilament(filament))
	{
		uint16_t bowdenLength = eeprom_read_word(&(eepromBase->eepromBowdenLen[filament]));

		if (eepromEmpty == bowdenLength)
		{
			const uint8_t LengthCorrectionLegacy = eeprom_read_byte(&(eepromBase->eepromLengthCorrection));
			if (LengthCorrectionLegacy <= 200)
			{
				bowdenLength = eepromLengthCorrectionBase + LengthCorrectionLegacy * 10;
			}
		}
		if (validBowdenLen(bowdenLength)) return bowdenLength;
	}

	return eepromBowdenLenDefault;
}


//! @brief Construct BowdenLength object which allows bowden length manipulation
//!
//! To be created on stack, new value is permanently stored when object goes out of scope.
//! Active filament and associated bowden length is stored in member variables.
BowdenLength::BowdenLength() : m_filament(active_extruder), m_length(BowdenLength::get())
{
}

//! @brief Increase bowden length
//!
//! New value is not stored immediately. See ~BowdenLength() for storing permanently.
//! @retval true passed
//! @retval false failed, it is not possible to increase, new bowden length would be out of range
bool BowdenLength::increase()
{
	if ( validBowdenLen(m_length + stepSize))
	{
		m_length += stepSize;
		return true;
	}
	return false;
}

//! @brief Decrease bowden length
//!
//! New value is not stored immediately. See ~BowdenLength() for storing permanently.
//! @retval true passed
//! @retval false failed, it is not possible to decrease, new bowden length would be out of range
bool BowdenLength::decrease()
{
	if ( validBowdenLen(m_length - stepSize))
	{
		m_length -= stepSize;
		return true;
	}
	return false;
}

//! @brief Store bowden length permanently.
BowdenLength::~BowdenLength()
{
	if (validFilament(m_filament))eeprom_update_word(&(eepromBase->eepromBowdenLen[m_filament]), m_length);
}

//! @brief Erase whole EEPROM
void BowdenLength::eraseAll()
{
	for (uint16_t i = 0; i < 1024; i++)
	{
		eeprom_update_byte((uint8_t*)i, static_cast<uint8_t>(eepromEmpty));
	}
}
