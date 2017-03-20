/*
 * IrPwm.c
 *
 * Created: 16.03.2017 13:12:24
 * Author : ekrashtan
 * PB0 - PWM out
 * PB1 - IR input
 * B3,4 - analyzer
 *
 * L-Fuse: 7A
 *
 * RC5 (36KHz Phillips protocol) Decoding library for AVR
 * Copyright (c) 2011 Filip Sobalski <pinkeen@gmail.com>
 * based on the idea presented by Guy Carpenter
 * on http://www.clearwater.com.au/rc5
 */ 

#define F_CPU 9600000UL
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define IR_PIN  1

/* The formula to calculate ticks is as follows
 * TICKS = PULSE_LENGTH / (1 / (CPU_FREQ / TIMER_PRESCALER))
 * Where CPU_FREQ is given in MHz and PULSE_LENGTH in us.
 * LONG_MIN should usually be SHORT_MAX + 1 */
#define IR_SHORT_MIN 16   /* 444 microseconds */
#define IR_SHORT_MAX 49  /* 1333 microseconds */
#define IR_LONG_MIN 50   /* 1334 microseconds */
#define IR_LONG_MAX 82   /* 2222 microseconds */
#define SLEEP_DELAY 82   /* 2222 microseconds */

#define RC5_GetStartBits(command) ((command & 0x3000) >> 12)
#define RC5_GetCommandBits(command) (command & 0x3F)

typedef enum {
    STATE_START1,
    STATE_MID1,
    STATE_MID0,
    STATE_START0,
    STATE_ERROR,
    STATE_BEGIN,
    STATE_END
} State;

const uint8_t trans[4] = {0x01, 0x91, 0x9b, 0xfb};
volatile uint16_t command;
uint8_t ccounter;
volatile uint8_t has_new;
State state = STATE_BEGIN;
volatile uint16_t Clock27us = 0;

void RC5_Reset()
{
	has_new = 0;
	ccounter = 14;
	command = 0;
	state = STATE_BEGIN;

	/* Enable INT0 */
	GIMSK |= _BV(INT0);
}

uint8_t RC5_NewCommandReceived(uint16_t *new_command)
{
	if(has_new)
	{
		*new_command = command;
	}

	return has_new;
}

ISR(INT0_vect)
{
	PORTB ^= 0x10;
	uint16_t delay = Clock27us;
	/* TSOP2236 pulls the data line up, giving active low,
     * so the output is inverted. If data pin is high then the edge
     * was falling and vice versa.
     *
     *  Event numbers:
     *  0 - short space
     *  2 - short pulse
     *  4 - long space
     *  6 - long pulse
     */
    uint8_t event = (PINB & _BV(PINB0)) ? 2 : 0;

    if(delay > IR_LONG_MIN && delay < IR_LONG_MAX)
    {
        event += 4;
    }
    else if(delay < IR_SHORT_MIN || delay > IR_SHORT_MAX)
    {
        /* If delay wasn't long and isn't short then
         * it is erroneous so we need to reset but
         * we don't return from interrupt so we don't
         * loose the edge currently detected. */
        RC5_Reset();
    }

	if(state == STATE_BEGIN)
	{
		ccounter--;
		command |= 1 << ccounter;
		state = STATE_MID1;
		Clock27us = 0;
		return;
	}

	State newstate = (trans[state] >> event) & 0x03;

	if(newstate == state || state > STATE_START0)
    {
        /* No state change or wrong state means
         * error so reset. */
        RC5_Reset();
        return;
    }

    state = newstate;

    /* Emit 0 - jest decrement bit position counter
     * cause data is already zeroed by default. */
    if(state == STATE_MID0)
    {
        ccounter--;
    }
    else if(state == STATE_MID1)
    {
        /* Emit 1 */
        ccounter--;
        command |= 1 << ccounter;
    }

    /* The only valid end states are MID0 and START1.
     * Mid0 is ok, but if we finish in MID1 we need to wait
     * for START1 so the last edge is consumed. */
    if(ccounter == 0 && (state == STATE_START1 || state == STATE_MID0))
    {
	    state = STATE_END;
	    has_new = 1;

	    /* Disable INT0 */
	    GIMSK &= ~_BV(INT0);
    }

    Clock27us = 0;
}

void HW_init(void)
{
	// TMR0 fast PWM mode
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);   // clock source = CLK, start PWM
	// Enable IRQ
	TIMSK0 |= _BV(TOIE0);
	// PWM out
	OCR0A = 100;
	// PB1 - input, PB0 - out
	DDRB |= _BV(PB3)|_BV(PB4)|_BV(PB0);
	/* Set INT0 to trigger on any edge */
	MCUCR |= _BV(ISC00);

}

// every 27uS
ISR(TIM0_OVF_vect)
{
	PORTB ^= 0x8;
	Clock27us ++;
}

int main(void)
{
	HW_init();
	RC5_Reset();
	sei();
	
    while (1) 
    {
		uint16_t command;

        /* Poll for new RC5 command */
        if(RC5_NewCommandReceived(&command))
        {
            /* Reset RC5 lib so the next command
             * can be decoded. This is a must! */
            RC5_Reset();
			if(RC5_GetStartBits(command) != 3)
			{
				// Ignore bad command
			} else {
				uint8_t cmdnum = RC5_GetCommandBits(command);
				switch (cmdnum)
				{
				case 16:
					OCR0A = 2;
					break;
				case 17:
					OCR0A = 200;
					break;
				default:
					break;
				}
			}
		}
    }
}

