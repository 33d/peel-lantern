#if !defined(SERIAL_H)
#define SERIAL_H

#include <stdio.h>

/*
To use, define these two macros:
#define SERIAL_BAUD 115200
// Use double-speed mode.  See p204 of the data sheet for the better error
// rate.  At 16MHz, recommended for 57600 and 115200.
#define SERIAL_U2X 1
*/

#if !defined(SERIAL_BAUD)
#error You need to set SERIAL_BAUD before including serial.h
#endif

#if defined(SERIAL_U2X)
#define UBRR_VAL ((F_CPU / 8 / SERIAL_BAUD) - 1)
#else
#define UBRR_VAL ((F_CPU / 16 / SERIAL_BAUD) - 1)
#endif

/* defined in serial.c */
extern FILE stdout_uart;

void serial_init() { 
  // transmitter enable
  UCSR0B |= _BV(TXEN0);

  stdout = &stdout_uart;
}

#endif

