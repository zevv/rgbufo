#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "printd.h"
#include "adc.h"
#include "pwm.h"


void led_set(uint8_t r, uint8_t g, uint8_t b);
void hsv2rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b);


struct rgb {
	uint8_t mask;
	uint8_t val;
};

volatile struct rgb rgb[3] = { 
	{ .mask = (1<<PC2) },
	{ .mask = (1<<PC1) },
	{ .mask = (1<<PC0) }
};

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void led_set_hsv(uint8_t h, uint8_t s, uint8_t v);


void flop(void)
{
	uint8_t c, i;
	volatile struct rgb *r = &rgb[0];

	for(c=0; c<3; c++) {
		PORTC &= ~r->mask;
		for(i=0; i<255; i++) if(i == r->val) PORTC |= r->mask;
		PORTC |= r->mask;
		r++;
	}
}



int main(void)
{
	uart_init(UART_BAUD(1200));

	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2);
	DDRB |= (1<<PB0);

	int i = 0;
	char buf[32];
	uint8_t r = 0, g = 0, b = 0;
	
	adc_init();
	pwm_init();

	TCCR0 = (1<<CS01);
	TIMSK |= (1<<TOIE0);

	sei();

	for(;;) {
		flop();

		int c = uart_rx();

		if(c != -1) {
			uart_tx(c);

			if(i < sizeof(buf) - 2) {
				buf[i] = c;
				buf[i+1] = 0;
				i++;
			}

			if(c == 13 || c == 10) {
				int t = atoi(buf+1);
				if(buf[0] == 'r') r = t;
				if(buf[0] == 'g') g = t;
				if(buf[0] == 'b') b = t;

				led_set_rgb(r, g, b);

				i = 0;
				buf[i] = 0;
			}
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
	
	//printd("h,s,v=%d,%d,%d r,g,b=%d,%d,%d i=%d f=%d m=%d n=%d\n", h, s, v, *r, *g, *b, i, f, m, n);

}


void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	rgb[0].val = r;
	rgb[1].val = g;
	rgb[2].val = b;
}


void led_set_hsv(uint8_t h, uint8_t s, uint8_t v)
{
	uint8_t r, g, b;
	hsv2rgb(h, s, v, &r, &g, &b);
	led_set_rgb(r, g, b);
}


ISR(TIMER0_OVF_vect)
{
	
#define BS 8

	static int8_t buf[BS];
	static uint8_t p = 0;
	static int16_t mavg = 0;
	
	TCNT0 = 53;	/* approx 9600 hz */

	uint8_t v = adc_sample(5) >> 2;

	buf[p] = v - 128;
	uint8_t pn = (p + 1) % BS;
	int16_t m = (buf[p] * buf[pn]) >> 8;
	mavg = (mavg * 3 + m * 1) >> 2;
	pwm_set(m + 128);

	if(mavg < 0) {
		PORTB |= 1;
	} else {
		PORTB &= ~1;
	}
	
	p = pn;

}

