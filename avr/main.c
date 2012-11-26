
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


void handle_char(uint8_t c);
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

	printd("Hello\n");

	sei();

	led_set_rgb(30, 30, 30);

	for(;;) {
		uint8_t i, j;

		for(i=0; i<3; i++) {
			volatile struct rgb *r = &rgb[i];
			for(j=0; j<2; j++) {
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


void handle_pkt(uint8_t *buf, uint8_t len)
{
	if(buf[0] == 'c') {
		led_set_rgb(buf[1], buf[2], buf[3]);
	}
}


void handle_char(uint8_t c)
{
	static uint8_t buf[32];
	static uint8_t p = 0;
	static uint8_t escape = 0;

	//uart_tx('a');

	if(escape) {
		if(p < sizeof(buf)-1) buf[p++] = c;
		escape = 0;
	} else {
		switch(c) {
			case 0xff:
				if(p > 1) {
					uint8_t sum = 0;
					uint8_t i;
					for(i=0; i<p-1; i++) {
						sum += buf[i];
					}
					sum ^= 0xff;
					if(sum == buf[p-1]) {
						handle_pkt(buf, p-1);
					}
				}
				p = 0;
				break;
			case 0xfe:
				escape = 1;
				break;
			default:
				if(p < sizeof(buf)-1) buf[p++] = c;
				break;
		}
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
	
	int16_t v = adc_sample(5);

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

