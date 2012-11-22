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
	uint8_t val;
};

volatile struct rgb rgb[3] = { 
	{ .mask = (1<<PC2) },
	{ .mask = (1<<PC1) },
	{ .mask = (1<<PC0) }
};

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void do_pwm(void);


int main(void)
{
	uart_init(UART_BAUD(1200));

	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2);
	DDRB |= (1<<PB0);
	DDRD |= (1<<PD7);

	int i = 0;
	char buf[32];
	uint8_t r = 0, g = 0, b = 0;
	
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

			if(i < sizeof(buf) - 2) {
				buf[i] = c;
				buf[i+1] = 0;
				i++;
			}

			if(c == 13 || c == 10) {
				if(buf[0] == 'd') {
					uart_tx(buf[1]);
				}
				if(buf[0] == 'c' && i > 7) {
					b = strtol(buf+5, NULL, 16);
					buf[5] = 0;
					g = strtol(buf+3, NULL, 16);
					buf[3] = 0;
					r = strtol(buf+1, NULL, 16);

					led_set_rgb(r, g, b);
				}

				i = 0;
				buf[i] = 0;
			}
		}
	}

	return 0;
}


void do_pwm(void)
{
	uint8_t c, i;
	volatile struct rgb *r = &rgb[0];

	for(c=0; c<3; c++) {
		if(r->val > 1) PORTC &= ~r->mask;
		for(i=0; i<255; i++) if(i == r->val) PORTC |= r->mask;
		PORTC |= r->mask;
		r++;
	}
}



void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	rgb[0].val = r;
	rgb[1].val = g;
	rgb[2].val = b;
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

	if(v > 0) bias ++;
	if(v < 0) bias --;

	v = (shift > 0) ? (v >> shift) : (v << -shift);
	
	if(v > max) max = v;
	if(max > 0) max --;

	static uint8_t n = 0;
	if(n-- == 0) {
		if(max >= 0x085) shift++;
		if(max <= 0x01f) shift--;
		if(shift > 4) shift = 4;
		if(shift < -4) shift = -4;
	}
	
	buf[p] = v; 
	uint8_t pn = (p + 1) % BS;
	int16_t m = (buf[p] * buf[pn]) >> 8;
	mavg = (mavg * 3 + m * 1) >> 2;
	pwm_set(mavg + 127);

	if(mavg < 0) {
		PORTB |= 1;
	} else {
		PORTB &= ~1;
	}
	
	p = pn;
	
	PORTD &= ~(1<<PD7);

}

