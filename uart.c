/**
 * \file uart.c
 * \brief Interrupt driven UART handling
 */

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

#define RB_SIZE 128

struct ringbuffer_t {
	uint8_t buf[RB_SIZE];
	uint8_t head;
	uint8_t tail;
};

int debug_putchar(char c);

volatile struct ringbuffer_t rb_rx;
volatile struct ringbuffer_t rb_tx;


/*
 * Initialize the UART. Should be called before any of the other uart functions
 * is used
 */
 
void uart_init(uint16_t baudrate)
{
	UBRRH = (baudrate >> 8);			/* Set the baudrate [p.132] */
	UBRRL = (baudrate & 0xff);			
	UCSRB = (1<<RXCIE) | (1<<RXEN) | (1<<TXEN);	/* Enable receiver and transmitter and rx interrupts [p.136] */
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);	/* Set to no parity, 8 data bits, 1 stopbit */

}


/*
 * Write one character to the transmit ringbuffer
 */ 
 
int uart_tx(char c)
{
	uint8_t head;
	head = (rb_tx.head + 1) % sizeof(rb_tx.buf);
	
	while(rb_tx.tail == head);	/* Wait for free space in the buffer */
	rb_tx.buf[rb_tx.head] = c;	/* Put the char in the ringbuffer */
	rb_tx.head = head;		/* Increase head ptr */
	UCSRB |= (1<<UDRIE);		/* Enable TX-reg empty isr */

	return 0;
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


/*
 * RX interrupt function : Read one byte from uart and put into the rx
 * ringbuffer. If the RB is full, the new data will be dropped.
 */
 
ISR(USART_RXC_vect)
{
	uint8_t head;
	
	head = (rb_rx.head + 1) % sizeof(rb_rx.buf);
	if(rb_rx.tail == head) return;
	
	rb_rx.buf[rb_rx.head] = UDR;		/* Get data from UART and put into rb */
	rb_rx.head = head;			/* Update ringbuffer head pointer */
}


/*
 * TX-register empty interrupt 
 */
 
ISR(USART_UDRE_vect)
{
	UDR = rb_tx.buf[rb_tx.tail];				/* Put char in TX reg */
	rb_tx.tail = (rb_tx.tail + 1) % sizeof(rb_tx.buf);	/* Update ringbuffer */
	
	if(rb_tx.tail == rb_tx.head) {				/* If rb empty, disable irq */
		UCSRB &= ~(1<<UDRIE);
	}
}

/*
 * End
 */
