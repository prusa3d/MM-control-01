/*
 * timer.h
 *
 * Created: 1/16/2019 9:42:32 PM
 *  Author: warlo
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <avr/io.h>

// we got 2 16-bit timers lets use them for god sake
class mutex {
	uint8_t _flag;
public:
	mutex() : _flag(SREG) { cli(); }
	~mutex() { SREG = _flag; }		
	void wait() { __asm("sei\n    nop\n    cli" : : : "memory"); }
};

constexpr inline uint32_t ticks_from_us(uint32_t us) { return us * (F_CPU / 1000000); }
constexpr inline uint32_t ticks_from_ms(uint32_t us) { return us * (F_CPU / 1000); }
// Return true if time1 is before time2.  Always use this function to
// compare times as regular C comparisons can fail if the counter
// rolls over.
static uint8_t inline timer_is_before(const uint32_t time1, const  uint32_t time2)
{
	union u32_u {
		struct { uint8_t b0, b1, b2, b3; };
		struct { uint16_t lo, hi; };
		uint32_t val;
	};
	// This asm is equivalent to:
	//     return (int32_t)(time1 - time2) < 0;
	// But gcc doesn't do a good job with the above, so it's hand coded.
	union u32_u utime1 = { .val = time1 };
	uint8_t f = utime1.b3;
	asm("    cp  %A1, %A2\n"
	"    cpc %B1, %B2\n"
	"    cpc %C1, %C2\n"
	"    sbc %0,  %D2"
	: "+r"(f) : "r"(time1), "r"(time2));
	return (int8_t)f < 0;
}


#endif /* TIMER_H_ */