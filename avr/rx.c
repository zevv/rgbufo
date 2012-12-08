
//448 uS

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

#include "uart.h"
#include "printd.h"
#include "adc.h"
#include "pwm.h"


void rx_init(void)
{
	TCCR0B = (1<<CS01);
	TIMSK0 |= (1<<TOIE0);
}


#define HIST_LEN 12	/* number of samples in correlator history */
#define FILT_LEN 22	/* number of samples in moving average filter */

ISR(TIMER0_OVF_vect)
{
	static int16_t bias = 512;
	static uint8_t rb_filt[FILT_LEN] = { 0 };
	static uint8_t p_filt = 0;
	static uint8_t filt_tot = 0;
	static uint8_t n = 0;
	static uint16_t hist = 0;

	__asm("nop");
	TCNT0 = 0xb9; /* 26.4 khz */
	
	int16_t v = adc_sample(1);

	/* Remove DC */

	if(n++ == 0) {
		if(v > bias) bias ++;
		if(v < bias) bias --;
	}

	/* Convert sampled value to square wave */

	uint8_t w = v > bias;

	/* Save HIST_LEN samples of history */

	hist = (hist>>1) | (w<<HIST_LEN);
	uint8_t m = (w ^ hist) & 1;
	
	/* Correlator: multiply sample with delayed sample, uses xor as cheap
	 * multiply */
	
	uint8_t p_filt_next = p_filt + 1;
	if (p_filt_next >= HIST_LEN) p_filt_next = 0;
	rb_filt[p_filt] = m;
	filt_tot += rb_filt[p_filt] - rb_filt[p_filt_next];
	p_filt = p_filt_next;

	pwm_set(filt_tot * 8 + 128);

	/* Output data to port looped back to uart RX */

#define DISC 6

	if(filt_tot < DISC) {
		PORTD |= (1<<PD2);
	} else {
		PORTD &= ~(1<<PD2);
	}
}


/*
 * End
 */

