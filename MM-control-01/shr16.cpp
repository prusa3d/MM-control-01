//shr16.c - 16bit shift register (2x74595)

#include "shr16.h"
#include <avr/io.h>

#include "Buttons.h"
#include "twi.h"

const int ButtonPin = A2;
uint16_t shr16_v;

static void shr16_write(uint16_t v);

Btn buttonClicked()
{
	#if 0
	int raw = analogRead(ButtonPin);

	if (raw < 50) return Btn::right;
	if (raw > 80 && raw < 100) return Btn::middle;
	if (raw > 160 && raw < 180) return Btn::left;
	#endif

	uint8_t tmp;
	twi_readFrom(0x21,&tmp,1,1);
	tmp >>=5;
	if(tmp !=7) {
		uint8_t cmp;
		delay(10);
		twi_readFrom(0x21,&cmp,1,1);
		cmp >>=5;
		if(tmp == cmp) {
			switch(tmp) {
				case 0b011: return Btn::left;
				case 0b101: return Btn::middle;
				case 0b110: return Btn::right;
			}
		}
	}
	#if 0
		static uint8_t button = 7;
			twi_readFrom(0x21,&tmp,1,1);
	tmp >>=5;
	if(tmp != button) {
		button = tmp;
		switch(tmp) {
			case 0b011: return Btn::left;
			case 0b101: return Btn::middle;
			case 0b110: return Btn::right;
		}
	}
	#endif
	return Btn::none;
}
 enum class Led : uint16_t{ 
	OFF = 0,
	G0 = (1<<0),
	Y0 = (1<<1),
	G1 = (1<<2),
	Y1 = (1<<3),
	G2 = (1<<4),
	Y2 = (1<<5),
	G3 = (1<<6),
	Y3 = (1<<7),
	G4 = (1<<8),
	Y4 = (1<<9),
	GALL = G0 | G1 | G2 | G3 | G4,
	YALL = Y0 |Y1 | Y2 | Y3 | Y4,
};
static bool have_remote = false;
#if 0
void i2c_scanner() {
	byte error, address;
	int nDevices;
	
	Serial1.println("Scanning...");
	
	nDevices = 0;
	for(address = 1; address < 127; address++ )
	{
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		
		if (error == 0)
		{
			Serial1.print("I2C device found at address 0x");
			if (address<16)
			Serial1.print("0");
			Serial1.print(address,HEX);
			Serial1.println("  !");
			
			nDevices++;
		}
		else if (error==4)
		{
			Serial1.print("Unknown error at address 0x");
			if (address<16)
			Serial1.print("0");
			Serial1.println(address,HEX);
		}
	}
	if (nDevices == 0)
	Serial1.println("No I2C devices found\n");
	else
	Serial1.println("done\n");
	
	delay(5000);           // wait 5 seconds for next scan
}
#endif

static uint16_t leds;

void start_test_i2c() {
	twi_init();
	have_remote = true;
	leds=0xFFFF;
	#if 0
	Wire.begin();
	Wire.setClock(400000);
	delay(1);
	Wire.beginTransmission(0x20);
	int error = Wire.endTransmission();
	if(error ==0) {
		Serial1.println("Found device at 0x20");
		have_remote = true;
	} else {
				Serial1.println("No device at 0x20");
				have_remote = false;
				i2c_scanner() ;
	}
	#endif
}

void shr16_init(void)
{
	DDRC |= 0x80;
	DDRB |= 0x40;
	DDRB |= 0x20;
	PORTC &= ~0x80;
	PORTB &= ~0x40;
	PORTB &= ~0x20;
	shr16_v = 0;
	shr16_write(shr16_v);
	shr16_write(shr16_v);

	have_remote=true;
	start_test_i2c();
	Serial1.println("Configured");
}

void shr16_write(uint16_t v)
{
	if(v == shr16_v) return;
	PORTB &= ~0x40;
	asm("nop");
	for (uint16_t m = 0x8000; m; m >>= 1)
	{
		if (m & v)
			PORTB |= 0x20;
		else
			PORTB &= ~0x20;
		PORTC |= 0x80;
		asm("nop");
		PORTC &= ~0x80;
		asm("nop");
	}
	PORTB |= 0x40;
	asm("nop");
	shr16_v = v;
}

void shr16_set_led(uint16_t led)
{
	//shr16_write((shr16_v & ~SHR16_LED_MSK) | ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2));
	if(have_remote){
		led ^= 0xFFFF;
		if(led == leds) return;
		leds = led;
		twi_writeTo(0x20,((uint8_t*)&leds),1,1,1);
		twi_writeTo(0x21,((uint8_t*)&leds)+1,1,0,1);
		#if 0
		i2c_txn_init(led_ops, NUM_OF_OPS);
		i2c_op_init_wr(&led_ops->ops[0], 0x20, reinterpret_cast<uint8_t*>(&leds),1);
		i2c_op_init_wr(&led_ops->ops[1], 0x21, reinterpret_cast<uint8_t*>(&leds)+1,1);
		i2c_post(led_ops);
				while (!(led_ops->flags & I2C_TXN_DONE)); // waiting for it to finish
				if (led_ops->flags & I2C_TXN_ERR) {
					Serial1.println("Fucking error!");
				}
				#endif
	} 
		//led = ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2);
		//shr16_write((shr16_v & ~SHR16_LED_MSK) | ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2));

}

void shr16_set_ena(uint8_t ena)
{
	ena ^= 7;
	ena = ((ena & 1) << 1) | ((ena & 2) << 2) | ((ena & 4) << 3); // 0. << 1 == 1., 1. << 2 == 3., 2. << 3 == 5.
	shr16_write((shr16_v & ~SHR16_ENA_MSK) | ena);
}

void shr16_set_dir(uint8_t dir)
{
	dir = (dir & 1) | ((dir & 2) << 1) | ((dir & 4) << 2); // 0., 1. << 1 == 2., 2. << 2 == 4.
	shr16_write((shr16_v & ~SHR16_DIR_MSK) | dir);
}

uint8_t shr16_get_ena(void)
{
	return ((shr16_v & 2) >> 1) | ((shr16_v & 8) >> 2) | ((shr16_v & 0x20) >> 3);
}

uint8_t shr16_get_dir(void)
{
	return (shr16_v & 1) | ((shr16_v & 4) >> 1) | ((shr16_v & 0x10) >> 2);
}
