/*
 * test2.c
 *
 * Created: 13.03.2017 17:05:03
 * Author : ekrashtan
 *
 * PD2/INT0/PCINT13 - infrared input
 * PD5/OC0B - PWM out
 */ 

#define F_CPU 8000000L
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "rc5.h"
#include "main.h"

void PWM_init(void)
{
	// TMR0 fast PWM mode
	TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS01);   // clock source = CLK/8, start PWM
	// PWM out
	DDRD |= _BV(PD5);
}

static char speedTable[] = {
	255, 192, 128, 64, 
	16, 4, 0, 0, 
	0, 0, 4, 16, 
	64, 128, 192, 255
};

#define SPEEDTABLELEN 16

void setSpeed(int chg)
{
	static char speed = 0;
	if (chg) {
		if (speed < SPEEDTABLELEN*2-1)
			speed ++;
	} else {
		if (speed > 0)
			speed --;
	}
	// set dir
	//PORTD &= 0xEF;
	//if (speed > SPEEDTABLELEN/2) {
	//	PORTD |= _BV(PD4);
	//}
	// set PWM
	OCR0B = speedTable[speed>>1];
}

int main(void)
{
	ACSR |= _BV(ACD); // Analog comp disable

	int toSleep = 0;
	PWM_init();
	RC5_Init();

	// analyzer
    DDRD |= _BV(PD0)|_BV(PD1)|_BV(PD4);

	// TMR1 normal mode
	TCCR1A = 0;
	TCCR1B = _BV(CS12);   // clock source = CLK/256

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

			} else {
				uint8_t cmdnum = RC5_GetCommandBits(command);
				switch (cmdnum)
				{
				case 16:
					setSpeed(1);
					break;
				case 17:
					setSpeed(0);
					break;
				default:
					break;
				} 
			}
			toSleep = 0;
		} else {
			_delay_ms(10);
			// counter to sleep
			toSleep ++;
			if (toSleep>512) {
				set_sleep_mode(SLEEP_MODE_STANDBY);
				sleep_enable();
				sei();
				sleep_cpu(); 
			}
		}		
    }
}

