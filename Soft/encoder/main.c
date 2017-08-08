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
#include <avr/sleep.h>

#define ENCA PB3
#define ENCB PB4
#define BUTTON PB4
#define DIR PB2

#define MAXSPEED 12
volatile uint8_t CurSpeed = MAXSPEED/2;
static const uint8_t SpeedTable[MAXSPEED] =
{ 255, 235, 205, 160, 128, 0, 255, 128, 95, 50, 20, 0 };
static uint8_t BtnPress = 0;
static uint8_t IsActive = 0;

static void PwmOff(void)
{
	// PWM off
	PORTB &= ~(_BV(DIR));
	OCR0A = 0;
}

static void SetSpeed(void)
{
	if (CurSpeed<MAXSPEED/2) {
		PORTB &= ~(_BV(DIR));
	} else {
		PORTB |= _BV(DIR);
	}
	OCR0A = SpeedTable[CurSpeed];
}

ISR(TIM0_OVF_vect)
{
	if (BtnPress) {
		BtnPress --;
		if (!BtnPress) {
			if (IsActive) {
				IsActive = 0;
				PwmOff();
			} else {
				IsActive = 1;
				SetSpeed();
			}
		}
	}
}

ISR (PCINT0_vect)
{
	//PORTB ^= 0x4;
	if (PINB & _BV(ENCA)) {
		SetSpeed();
	} else {
		if (PINB & _BV(ENCB)) {
			if (CurSpeed < MAXSPEED-1) {
				CurSpeed ++;
			}
		} else {
			if (CurSpeed > 0) {
				CurSpeed --;
			}
		}
	}
}

ISR (INT0_vect)
{
	BtnPress = 10;
}

void HwInit(void)
{
	ACSR |= _BV(ACD);

	// TMR0 fast PWM mode
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);   // clock source = CLK, start PWM


	// PB1 - btn, PB0 - PWM out, PB2 - Dir, PB3,4 - Encoder
	DDRB |= _BV(PB2)|_BV(PB0);
	PORTB |= _BV(BUTTON)|_BV(ENCA)|_BV(ENCB);

	// Configure pin change interrupts on PB3
	PCMSK = _BV(PCINT3);
	GIMSK = _BV(PCIE);                // Enable pin change interrupts

	// Configure INT0 interrupt (button)
	GIMSK |= _BV(INT0);
	// A falling edge should trigger the interrupt
	MCUCR |= _BV(ISC00)|_BV(ISC01);

	// Enable timer IRQ
	TIMSK0 |= _BV(TOIE0);
}

int main(void)
{
	HwInit();
	sei();

	PwmOff();

	while (1)
	{

	}
	return 0;
}

