#if !defined(SERIAL_H)
#define SERIAL_H

#include <stdio.h>

/* defined in serial.c */
extern FILE stdout_uart;

void serial_init() { 
  // transmitter only
  UCSR0B |= _BV(TXEN0);

  stdout = &stdout_uart;
}

#endif

