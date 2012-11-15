#include <stdint.h>
#include <avr/io.h>

void pwm_init(void)
{
	
	TCCR1A = (1<<WGM11) | (1<<COM1A1) | (1<<COM1B1);
	TCCR1B = (1<<WGM13) | (1<<WGM12)  | (1<<CS10);
	ICR1   = 32;

	DDRB |= (1<<PB1) | (1<<PB2);
}

void pwm_set(uint8_t val)
{
	OCR1A = val >> 3;
	OCR1B = val;
}
		
