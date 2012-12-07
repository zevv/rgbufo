
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

#define RB_SIZE 32

struct ringbuffer_t {
	uint8_t head;
	uint8_t tail;
	uint8_t buf[RB_SIZE];
};

static volatile struct ringbuffer_t rb = { 0, 0 };
static volatile struct ringbuffer_t rb_rx = { 0, 0 };

static int uart_putc(char c, FILE *f)
{
	uart_tx(c);
	return 0;
}

/*
 * Initialize the UART. 
 */

void uart_init(uint16_t baudrate_divider)
{
	rb.head = rb.tail = 0;

	/* Set baudrate, no parity, 8 databits 1 stopbit */

	UBRR0H = (baudrate_divider >> 8);
	UBRR0L = (baudrate_divider & 0xff);			
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);

	/* Enable receiver and transmitter and rx interrupts */

	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);	

	/* Uart is stdout */

	fdevopen(uart_putc, NULL);
}


/*
 * Write one character to the transmit ringbuffer and enable TX-reg-empty interrupt
 */ 
 
void uart_tx(uint8_t c)
{
	uint8_t head;
	head = (rb.head + 1) % RB_SIZE;
//	while(rb.tail == head);

	rb.buf[rb.head] = c;
	rb.head = head;

	UCSR0B |= (1<<UDRIE0);
}

/*
 * Read one character from the UART [p.140]
 */

int uart_rx(void)
{	
	uint8_t d;
	
	if(rb_rx.head == rb_rx.tail) return -1;

	d = rb_rx.buf[rb_rx.tail];				/* Get data from ringbuffer */
	rb_rx.tail = (rb_rx.tail + 1) % sizeof(rb_rx.buf);	/* Update rb pointers */
	return d;						/* Return data */
}


ISR(USART_RX_vect)
{
	uint8_t head;
	
	head = (rb_rx.head + 1) % sizeof(rb_rx.buf);
	if(rb_rx.tail == head) return;
	
	rb_rx.buf[rb_rx.head] = UDR0;
	rb_rx.head = head;
}


ISR(USART_UDRE_vect)
{
	UDR0 = rb.buf[rb.tail];
	rb.tail = (rb.tail + 1) % RB_SIZE;
	
	if(rb.tail == rb.head) {
		UCSR0B &= ~(1<<UDRIE0);
	}
}


/*
 * End
 */

