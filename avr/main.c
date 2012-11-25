
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


struct rgb {
	uint8_t mask;
	uint8_t set;
	uint8_t val;
};


void handle_char(char c);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void do_pwm_p(void);
void do_pwm_s(void);


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

	led_set_rgb(30, 30, 30);

	for(;;) {
		uint8_t i, j;

		for(i=0; i<3; i++) {
			volatile struct rgb *r = &rgb[i];
			for(j=0; j<4; j++) {
				if(r->val < r->set) r->val ++;
				if(r->val > r->set) r->val --;
			}
		}

		do_pwm_p();

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


void do_pwm_p(void)
{
	uint8_t i, c;
		
	for(c=0; c<3; c++) {
		volatile struct rgb *r = &rgb[c];
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


void do_pwm_s(void)
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


ISR(TIMER0_OVF_vect)
{
	//TCNT0 = 0x6d; /* 13.2khz */
	__asm("nop");
	TCNT0 = 0xb9; /* 26.4 khz */
	
#define RB_CORR_LEN 12
#define RB_FILT_LEN 22

	static int16_t bias = 512;
	static uint8_t rb_filt[RB_FILT_LEN] = { 0 };
	static uint8_t p_filt = 0;
	static uint8_t filt_tot = 0;
	static uint8_t n = 0;

	PORTD |= (1<<PD7);

	int16_t v = adc_sample(5);

	/* Remove DC */

	if(n++ == 0) {
		if(v > bias) bias ++;
		if(v < bias) bias --;
	}

	/* Correlator: multiple sample with delayed sample, uses xor as cheap
	 * multiply */

	static uint16_t corrhist = 0;
	uint8_t w = v > bias;

	corrhist = (corrhist >> 1) | (w<<RB_CORR_LEN);
	uint8_t m = (w ^ corrhist) & 1;

	rb_filt[p_filt] = m;
	uint8_t p_filt_next = p_filt + 1;
	if (p_filt_next >= RB_CORR_LEN) p_filt_next = 0;
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
	
	PORTD &= ~(1<<PD7);
}


/*
 * End
 */

