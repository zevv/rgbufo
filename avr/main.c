
//448 uS

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/wdt.h>

#include "uart.h"
#include "printd.h"
#include "adc.h"
#include "pwm.h"
#include "rx.h"
#include "led.h"


void handle_char(uint8_t c);


int main(void)
{
	int i;
	wdt_disable();

	DDRB |= (1<<PB0) | (1<<PB1);
	DDRD |= (1<<PD7) | (1<<PD2);

	uart_init(UART_BAUD(1200));
	adc_init();
	rx_init();
	led_init();
	//pwm_init();

	for(i=0; i<256; i++) {
		led_set_hsv(i, 255, 10);
		led_pwm_p();
	}
	led_set_rgb(0, 0, 0);

	sei();

	for(;;) {
		led_pwm_p();

		int c = uart_rx();

		if(c != -1) {
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

/*
 * End
 */

