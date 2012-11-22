#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "printd.h"
#include "adc.h"
#include "pwm.h"


struct rgb {
	uint8_t mask;
	uint8_t set;
	uint8_t val;
};


void handle_char(char c);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void do_pwm(void);


volatile struct rgb rgb[3] = { 
	{ .mask = (1<<PC2) },
	{ .mask = (1<<PC1) },
	{ .mask = (1<<PC0) }
};


int main(void)
{
	uart_init(UART_BAUD(1200));

	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2);
	DDRB |= (1<<PB0);
	DDRD |= (1<<PD7) | (1<<PD2);

	adc_init();
	pwm_init();

	TCCR0 = (1<<CS01);
	TIMSK |= (1<<TOIE0);

	sei();

	for(;;) {
		do_pwm();

		int c = uart_rx();

		if(c != -1) {
			//uart_tx(c);
			handle_char(c);
		}
	}

	return 0;
}


void handle_char(char c)
{
	static int bufptr = 0;
	static char buf[32];

	if(c == 13 || c == 10) {

		if(buf[0] == 'd') {
			uart_tx(buf[1]);
		}

		if(buf[0] == 'c') {
			uint32_t c = strtol(buf+1, NULL, 16);
			uint8_t r = (c >> 16) & 0xff;
			uint8_t g = (c >>  8) & 0xff;
			uint8_t b = (c >>  0) & 0xff;
			led_set_rgb(r, g, b);
		}

		bufptr = 0;
		buf[0] = 0;

	} else if(bufptr < sizeof(buf) - 2) {

		buf[bufptr] = c;
		buf[bufptr+1] = 0;
		bufptr++;
	}
}


void do_pwm(void)
{
	uint8_t c, i;
	volatile struct rgb *r = &rgb[0];

	for(c=0; c<3; c++) {

		if(r->val < r->set) r->val ++;
		if(r->val > r->set) r->val --;

		if(r->val > 1) PORTC &= ~r->mask;
		for(i=0; i<255; i++) if(i == r->val) PORTC |= r->mask;
		PORTC |= r->mask;
		r++;
	}
}


void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	rgb[0].set = r;
	rgb[1].set = g;
	rgb[2].set = b;
}


ISR(TIMER0_OVF_vect)
{
	TCNT0 = 0xa0;
	
#define BS 16

	static int8_t buf[BS];
	static uint8_t p = 0;
	static int16_t mavg = 0;
	static int16_t max = 0;
	static int8_t shift = 5;
	static int16_t bias = 0;

	PORTD |= (1<<PD7);

	int16_t v = adc_sample(5) - bias;

	/* Remove DC */

	if(v > 0) bias ++;
	if(v < 0) bias --;

	/* Gain */

	v = (shift > 0) ? (v >> shift) : (v << -shift);

	/* Calculate gain */

	if(v > max) max = v;
	if(max > 0) max --;

	static uint8_t n = 0;
	if(n-- == 0) {
		if(max >= 0x085) shift++;
		if(max <= 0x01f) shift--;
		if(shift > 4) shift = 4;
		if(shift < -4) shift = -4;
	}

	/* Comb filter */
	
	buf[p] = v; 
	uint8_t pn = (p + 1) % BS;
	int16_t m = (buf[p] * buf[pn]) >> 8;
	mavg = (mavg * 3 + m * 1) >> 2;
	pwm_set(mavg + 127);
	p = pn;

	/* Output data to port looped back to uart RX */

	if(mavg < 0) {
		PORTD |= (1<<PD2);
	} else {
		PORTD &= ~(1<<PD2);
	}
	
	PORTD &= ~(1<<PD7);
}


/*
 * End
 */

