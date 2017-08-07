/*
 * main.c
 *
 *  Encoder+PWM
 * PB3, PB4 - encoder
 * PB0, PB2 - PWM
 * PB1 - button
 * PB5 - LED
 */

#ifdef F_CPU
#undef F_CPU
#endif
#define F_CPU 3800000UL

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ENCA PB3
#define ENCB PB4

static int8_t cur_speed = 0;

ISR(TIM0_OVF_vect)
{

}

ISR (PCINT3_vect)
{

	cur_speed += 10;
	OCR0A = cur_speed;
}

void HwInit(void)
{
	ACSR |= _BV(ACD);

	// TMR0 fast PWM mode
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);   // clock source = CLK, start PWM

	// PWM out
	OCR0A = 10;

	// PB1 - btn, PB0 - PWM out, PB2 - Dir, PB3,4 - Encoder
	DDRB |= _BV(PB2)|_BV(PB0);
	PORTB |= _BV(PB1)|_BV(PB3)|_BV(PB4);

	// Configure pin change interrupts on PB3
	PCMSK |= _BV(PCINT3);
	GIMSK = _BV(PCIE);                // Enable pin change interrupts
}

int main(void)
{
	HwInit();
	sei();

	while (1)
	{
		PORTB ^= 0x4;
	}
	return 0;
}

