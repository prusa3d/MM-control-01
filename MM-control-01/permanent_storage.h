//! @file
//! @author Marek Bel

#ifndef PERMANENT_STORAGE_H_
#define PERMANENT_STORAGE_H_

#include <stdint.h>
#include <avr/eeprom.h>
#define ARR_SIZE(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

//! @brief EEPROM data layout
//!
//! Do not remove, reorder or change size of existing fields.
//! Otherwise values stored with previous version of firmware would be broken.
//! It is possible to add fields in the end of this struct, ensure that erased EEPROM is handled well.
//! Last byte in EEPROM is reserved for layoutVersion. If some field is repurposed, layoutVersion
//! needs to be changed to force EEPROM erase.
typedef struct __attribute__ ((packed))
{
	uint8_t eepromLengthCorrection; //!< legacy bowden length correction
	uint16_t eepromBowdenLen[5];    //!< Bowden length for each filament
	uint8_t eepromFilamentStatus[3];//!< Majority vote status of eepromFilament wear leveling
	uint8_t eepromFilament[800];    //!< Top nibble status, bottom nibble last filament loaded
	uint8_t eepromDriveErrorCountH;
	uint8_t eepromDriveErrorCountL[2];
}eeprom_t;
static_assert(sizeof(eeprom_t) - 2 <= E2END, "eeprom_t doesn't fit into EEPROM available.");
//! @brief EEPROM layout version
static const uint8_t layoutVersion = 0xff;

//d = 6.3 mm        pulley diameter
//c = pi * d        pulley circumference
//FSPR = 200        full steps per revolution (stepper motor constant) (1.8 deg/step)
//mres = 2          pulley microstep resolution (uint8_t __res(AX_PUL))
//mres = 2          selector microstep resolution (uint8_t __res(AX_SEL))
//mres = 16         idler microstep resolution (uint8_t __res(AX_IDL))
//1 pulley ustep = (d*pi)/(mres*FSPR) = 49.48 um

static eeprom_t * const eepromBase = reinterpret_cast<eeprom_t*>(0); //!< First EEPROM address
static const uint16_t eepromEmpty = 0xffff; //!< EEPROM content when erased
static const uint16_t eepromLengthCorrectionBase = 7900u; //!< legacy bowden length correction base (~391mm)
static const uint16_t eepromBowdenLenDefault = 8900u; //!< Default bowden length (~427 mm)
static const uint16_t eepromBowdenLenMinimum = 6900u; //!< Minimum bowden length (~341 mm)
static const uint16_t eepromBowdenLenMaximum = 16000u; //!< Maximum bowden length (~792 mm)

void permanentStorageInit();
bool validFilament(uint8_t filament);

void eepromEraseAll();

//! @brief Read manipulate and store bowden length
//!
//! Value is stored independently for each filament.
//! Active filament is deduced from active_extruder global variable.
class BowdenLength
{
public:
	static uint16_t get();
	static const uint8_t stepSize = 10u; //!< increase()/decrease() bowden length step size
	BowdenLength();
	bool increase();
	bool decrease();
	~BowdenLength();

	uint8_t m_filament; //!< Selected filament
	uint16_t m_length;  //!< Selected filament bowden length
};

//! @brief Read and store last filament loaded to nozzle
//!
//! 800(data) + 3(status) EEPROM cells are used to store 4 bit value frequently
//! to spread wear between more cells to increase durability.
//!
//! Expected worst case durability scenario:
//! @n Print has 240mm height, layer height is 0.1mm, print takes 10 hours,
//!    filament is changed 5 times each layer, EEPROM endures 100 000 cycles
//! @n Cell written per print: 240/0.1*5/800 = 15
//! @n Cell written per hour : 15/10 = 1.5
//! @n Fist cell failure expected: 100 000 / 1.5 = 66 666 hours = 7.6 years
//!
//! Algorithm can handle one cell failure in status and one cell in data.
//! Status use 2 of 3 majority vote.
//! If bad data cell is detected, status is switched to next key.
//! Key alternates between begin to end and end to begin write order.
//! Two keys are needed for each start point and direction.
//! If two data cells fails, area between them is unavailable to write.
//! If this is first and last cell, whole storage is disabled.
//! This vulnerability can be avoided by adding additional keys
//! and start point in the middle of the EEPROM.
//!
//! It would be possible to implement twice as efficient algorithm, if
//! separate EEPROM erase and EEPROM write commands would be available and
//! if write command would allow to be invoked twice between erases to update
//! just one nibble. Such commands are not available in AVR Libc, and possibility
//! to use write command twice is not documented in atmega32U4 datasheet.
//!
class FilamentLoaded
{
public:
    static bool get(uint8_t &filament);
    static bool set(uint8_t filament);
private:
    enum Key
    {
        KeyFront1,
        KeyReverse1,
        KeyFront2,
        KeyReverse2,
        BehindLastKey,
    };
    static_assert (BehindLastKey - 1 <= 0xf, "Key does't fit into nibble.");
    static uint8_t getStatus();
    static bool setStatus(uint8_t status);
    static int16_t getIndex();
    static void getNext(uint8_t &status, int16_t &index);
    static void getNext(uint8_t &status);
};

class DriveError
{
public:
    static uint16_t get();
    static void increment();
private:
    static uint8_t getL();
    static void setL(uint8_t lowByte);
    static uint8_t getH();
    static void setH(uint8_t highByte);
};

#endif /* PERMANENT_STORAGE_H_ */
