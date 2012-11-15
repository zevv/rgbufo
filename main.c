#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "printd.h"
#include "adc.h"
#include "pwm.h"

volatile uint8_t r = 0;
volatile uint8_t g = 0;
volatile uint8_t b = 0;

uint8_t gamma_table[] = {
	0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
	8, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 20, 22, 24, 26, 28, 31, 34, 37, 40,
	44, 48, 52, 57, 62, 68, 74, 81, 89, 97, 106, 116, 126, 138, 150, 164, 179, 196,
	214, 234, 255,
};

void hsv2rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b);


int main(void)
{

	uart_init(UART_BAUD(9600));

	DDRD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
	PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7);

	DDRB |= (1<<PB0);

	TCCR0 = (1<<CS00);
	TIMSK |= (1<<TOIE0);

	adc_init();
	pwm_init();

	sei();

	char buf[32] = { 0 };
	uint8_t i = 0;

	_delay_ms(10);
	uart_tx('U');

	int h = 0;
	int s = 255;
	int v = 255;
	
	for(;;) {
	}

	for(;;) {

		char c = uart_rx();

		if(i < sizeof(buf) - 2) {
			buf[i] = c;
			buf[i+1] = 0;
			i++;
			uart_tx(c);
		}

		if(c == 13 || c == 10) {
			int t = atoi(buf+1);
			if(buf[0] == 'h') h = t;
			if(buf[0] == 's') s = t;
			if(buf[0] == 'v') v = t;

			uint8_t R, G, B;
			hsv2rgb(h, s, v, &R, &G, &B);
			r = gamma_table[R>>2]; 
			g = gamma_table[G>>2]; 
			b = gamma_table[B>>2]; 

			i = 0;
			buf[i] = 0;
		}
	}

	return 0;
}


void hsv2rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b)
{	
	uint16_t f, m, n;
	uint16_t i;
	
	h *= 6.0;
	i = h & 0xff00;
	f = h - i;
	if (!(i & 0x100)) f = 255 - f;
	m = (v * (0x100 - s)) >> 8;
	n = (v * (0x100 - ((s * f) >> 8))) >> 8;
	
	if(i<0x0000) i=0x0000;
	if(i>0x0600) i=0x0600;

	switch (i & 0xff00) {
		case 0x0600:
		case 0x0000: *r=v; *g=n, *b=m; break;
		case 0x0100: *r=n; *g=v, *b=m; break;
		case 0x0200: *r=m; *g=v, *b=n; break;
		case 0x0300: *r=m; *g=n, *b=v; break;
		case 0x0400: *r=n; *g=m, *b=v; break;
		case 0x0500: *r=v; *g=m, *b=n; break;
		default: *r = *g = *b = 255;
	}
	
	printd("h,s,v=%d,%d,%d r,g,b=%d,%d,%d i=%d f=%d m=%d n=%d\n", h, s, v, *r, *g, *b, i, f, m, n);

}



ISR(TIMER0_OVF_vect)
{
	static uint8_t c = 0;
	
	
	if(c == 0) PORTD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
	if(c == b) PORTD |= (1<<PD5);
	if(c == g) PORTD |= (1<<PD6);
	if(c == r) PORTD |= (1<<PD7);
	c++;

#define BS 10

	static uint8_t d = 0;

	if(d-- == 0) {
		d = 2;
	
		static uint8_t bias = 128;
		static uint8_t n = 0;
		static int8_t pb = 0;
		static uint8_t c = 0;

		int8_t v = adc_sample(0) >> 4;

		if(n-- == 0) {
			if(bias > v) bias --;
			if(bias < v) bias ++;
			n = 20;
		}
		
		int8_t b = v > bias;
		if(b != pb) {
			r = c;
			if(c < 8) {
				PORTB &= ~(1<<PB0);
			} else {
				PORTB |= (1<<PB0);
			}
			c = 0;
		}
	
		c++;
		pb = b;

	}

}

