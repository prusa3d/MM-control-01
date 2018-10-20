//! @file
//! @author Marek Bel

#ifndef PERMANENT_STORAGE_H_
#define PERMANENT_STORAGE_H_

#include <stdint.h>
//! @brief EEPROM data layout
//!
//! Do not remove, reorder or change size of existing fields.
//! Otherwise values stored with previous version of firmware would be broken.
//! It is possible to add fields in the end of this struct, ensure that erased EEPROM is handled well.
typedef struct
{
	uint8_t eepromLengthCorrection; //!< legacy bowden length correction
	uint16_t eepromBowdenLen[5];    //!< Bowden length for each filament
}eeprom_t;

static eeprom_t * const eepromBase = reinterpret_cast<eeprom_t*>(0); //!< First EEPROM address
static const uint16_t eepromEmpty = 0xffff; //!< EEPROM content when erased
static const uint16_t eepromLengthCorrectionBase = 7900u; //!< legacy bowden length correction base
static const uint16_t eepromBowdenLenDefault = 8900u; //!< Default bowden length
static const uint16_t eepromBowdenLenMinimum = 6900u; //!< Minimum bowden length
static const uint16_t eepromBowdenLenMaximum = 10900u; //!< Maximum bowden length

bool validFilament(uint8_t filament);


//! @brief Read manipulate and store bowden length
//!
//! Value is stored independently for each filament.
//! Active filament is deduced from active_extruder global variable.
class BowdenLength
{
public:
	static uint16_t get();
	static void eraseAll();
	static const uint8_t stepSize = 10u; //!< increase()/decrease() bowden length step size
	BowdenLength();
	bool increase();
	bool decrease();
	~BowdenLength();

public:
	uint8_t m_filament; //!< Selected filament
	uint16_t m_length;  //!< Selected filament bowden length
};




#endif /* PERMANENT_STORAGE_H_ */
