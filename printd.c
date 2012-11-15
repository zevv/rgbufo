#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "printd.h"
#include "uart.h"

/* 
 * Tiny printf only supporting ints 
 */

void printd(const char *fmt, ...)
{
	const char *p = fmt;
	va_list va;
	int val;
	char buf[6];

	va_start(va, fmt);

	while(*p) {
		if(*p == '%') {
			val = va_arg(va, int);
			itoa(val, buf, 10);
			printd(buf);
			p++;
		} else {
			uart_tx(*p);
		}
		p++;
	}

	va_end(va);
}

