
/*
 * Uart baudrate = fosc / (16 * (UBRR + 1)) [p.133] 
 *
 * UBRR = (fosc / baudrate) / 16 - 1
 *
 * These constants only apply when the xtal oscillator is 11.0592 Mhz !
 */

#include <stdint.h>
#include <stdlib.h>

#define UART_BAUD(baudrate)     ((F_CPU / baudrate) / 16 - 1)

void uart_init(uint16_t baudrate);
int uart_tx(char c);
int uart_rx(void);
void uart_puts(char *s);
void uart_printnum(uint16_t n);


/*
 * End
 */
