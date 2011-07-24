#include <avr/io.h>
#include <stdio.h>

int uart_putchar(char c, FILE* stream) {
  while (!(UCSR0A & (1 << UDRE0)) );
  UDR0 = c;
  return 0;
}

/* This line cannot be in a C++ file */
FILE stdout_uart = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);

