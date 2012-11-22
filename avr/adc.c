/**
 * \file adc.c
 * \brief A/D converter functions
 */

#include <avr/io.h>

/**
 * Initialize the A/D converter. Call before using adc_sample
 */
 
void adc_init(void)
{
	ADCSRA = (1<<ADEN) | (1<<ADPS2);	/* Enable A/D converter, 16 x divisor */
}

/**
 * Perform one A/D conversion on the given input
 *
 * \param input	Input number
 * \return	The A/D conversion result, 10 bits value
 */
 
uint16_t adc_sample(uint8_t input)
{
	static int started = 0;
	uint16_t value;

	ADMUX = input;
	if(started) while(!(ADCSRA & (1<<ADIF)));	/* Wait until complete (probably done already) */
	value = ADC;					/* Get result from previous conversion */
	ADCSRA |= (1<<ADSC);				/* Start new conversion */
	started = 1;
	
	return value;
}

/*
 * End
 */
