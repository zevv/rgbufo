
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
