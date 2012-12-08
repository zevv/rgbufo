
//448 uS

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/wdt.h>

#include "printd.h"
#include "led.h"


struct rgb {
	uint8_t mask;
	uint8_t set;
	uint8_t val;
};

static void hsv2rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b);

struct rgb rgb[3] = { 
	{ .mask = (1<<PC4) },
	{ .mask = (1<<PC3) },
	{ .mask = (1<<PC5) }
};


void led_init(void)
{
	DDRC |= (1<<PC3) | (1<<PC4) | (1<<PC5);
}


void led_pwm_p(void)
{
	uint8_t i, c, j;

	for(c=0; c<3; c++) {
		volatile struct rgb *r = &rgb[c];
		for(j=0; j<2; j++) {
			if(r->val < r->set) r->val ++;
			if(r->val > r->set) r->val --;
		}
		if(r->val) PORTC |= r->mask;
	}

	for(i=0; i<255; i++) {
		for(c=0; c<3; c++) {
			volatile struct rgb *r = &rgb[c];
			if(i == r->val) {
				PORTC &= ~r->mask;
			}
		}
	}
}


void led_pwm_s(void)
{
	uint8_t c, i;
	volatile struct rgb *r = &rgb[0];

	for(c=0; c<3; c++) {
		if(r->val > 1) PORTC |= r->mask;
		for(i=0; i<255; i++) if(i == r->val) PORTC &= ~r->mask;
		PORTC &= ~r->mask;
		r++;
	}
}


void led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	rgb[0].set = r;
	rgb[1].set = g;
	rgb[2].set = b;
}


void led_set_hsv(uint8_t h, uint8_t s, uint8_t v)
{
	uint8_t r, g, b;
	hsv2rgb(h, s, v, &r, &g, &b);
	led_set_rgb(r, g, b);
}


static void hsv2rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b)
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
}



/*
 * End
 */

