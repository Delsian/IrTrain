/*
 * main.c
 *
 *  NEC IR protocol
 */

#ifdef F_CPU
#undef F_CPU
#endif
#define F_CPU 9600000UL

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Predefined address of remote
#define IR_ADDRESS 0
#define SLEEP_TIMEOUT 100

#define LONG_HEADER 310 /* 9ms */
#define SHORT_HEADER 130 /* 4.5ms */
#define LONG_PULSE 50 /* 1690us */
#define SHORT_PULSE 12 /* 562us */
volatile uint16_t Clock27us = 0;
volatile static uint8_t IR_Addr, IR_Command, IR_counter, IR_tmp, IR_bit;
volatile uint8_t has_new;
volatile uint16_t to_sleep;
static uint8_t cur_speed = 0;

void IR_Reset()
{
	has_new = IR_counter = 0;
	/* Enable INT0 */
	GIMSK |= _BV(INT0);
}

ISR(INT0_vect)
{
	// Wake from sleep
	to_sleep = SLEEP_TIMEOUT;
	// Enable timer IRQ
	TIMSK0 |= _BV(TOIE0);

	Clock27us = 0;
	if (IR_bit == 0) {
		IR_Reset(); // Pulse too short
		return;
	}
	if (PINB & _BV(PINB1))
	{
		if (IR_bit == 4) {
			IR_counter = 0x40;
		} else if (IR_bit != 1) {
			IR_Reset(); // Zero pulses always short
		}
	} else {
		if (!IR_counter) {
			return; // Waiting for start
		}
		if (IR_counter&0x40) {
			IR_counter = (IR_bit == 3)? 0x80:0; // Is Header pulse?
		} else {
			IR_counter ++;
			IR_tmp <<= 1;
			IR_tmp += (IR_bit == 2)? 1:0;

			if (IR_counter & 7) return;

			switch(IR_counter & 0x38) {
			case 0x8:
				IR_Addr = IR_tmp;
				break;
			case 0x10:
				if (IR_Addr != (IR_tmp^0xFF)) {
					IR_Reset(); // Wrong inverse decoding
				}
				break;
			case 0x18:
				IR_Command = IR_tmp;
				break;
			case 0x20:
				if (IR_Command == (IR_tmp^0xFF)) {
					has_new = 1;
					GIMSK &= ~(_BV(INT0));
				}
				break;
			default:
				break;
			}
		}
	}


}
//====

ISR(TIM0_OVF_vect)
{
	to_sleep --;
	if (Clock27us > LONG_HEADER) {
		Clock27us = 0xFFFF;
		IR_bit = 4;
	} else {
		Clock27us ++;
		IR_bit = 0;
		if (Clock27us > SHORT_HEADER) {
			IR_bit = 3;
		} else if (Clock27us > LONG_PULSE) {
			IR_bit = 2;
		} else if (Clock27us > SHORT_PULSE) {
			IR_bit = 1;
		}
	}
}

void HW_init(void)
{
	// TMR0 fast PWM mode
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);   // clock source = CLK, start PWM

	// PWM out
	OCR0A = 0;
	// PB1 - input, PB0 - PWM out, PB2 - Dir, PB3 - LED
	DDRB |= _BV(PB3)|_BV(PB4)|_BV(PB2)|_BV(PB0);
	/* Set INT0 to trigger on any edge */
	MCUCR |= _BV(ISC00);

}

// +1 - one step up
// -1 - one step down
// 0 - stop
static void set_speed(int8_t delta)
{
	const uint8_t steps[4] = {1,80, 170, 255};
	cur_speed += delta;
	OCR0A = steps[cur_speed&3];
	if (cur_speed & 4) {
		PORTB &= ~0x4;
	} else {
		PORTB |= 0x4;
	}
}

int main(void)
{
	HW_init();
	IR_Reset();
	sei();

    while (1)
    {
    	if(to_sleep<5) {
    		PORTB ^= 0x10;
    	}
    	if(has_new) {
    		if (IR_Addr == IR_ADDRESS) {
    			switch(IR_Command) {
    			case 0x22: // <-
    				set_speed(1);
    				break;
    			case 0xC2: //->
    				set_speed(-1);
    				break;
    			case 0x2: // stop
    				set_speed(0);
    				break;

    			default:
    				break;
    			}
    		}
    		IR_Reset();
    	}
    }
	return 0;
}

