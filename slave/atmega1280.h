#if !defined(__PEEL_ATMEGA1280_H__)
#define __PEEL_ATMEGA1280_H__

#include <stdio.h>

#define RX_vect USART1_RX_vect
#define UDR UDR1
#define UCSRA UCSR1A
#define UCSRB UCSR1B
#define RXB8 RXB81
#define MPCM MPCM1

#if !defined(PEEL_BAUD)
#error Define PEEL_BAUD first
#endif

#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

void peel_serial_init() {
	  UBRR1H = (uint8_t) (PEEL_UBRR_VAL >> 8);
	  UBRR1L = (uint8_t) (PEEL_UBRR_VAL);
	  // Enable rx, and interrupt for rx
	  UCSR1B = _BV(RXEN1) | _BV(RXCIE1)
	  // Asynchronous, no parity, 1 stop bit, 9 data bits (9th bit is the address
	  // bit for multi-processor mode)
	     | _BV(UCSZ12);
	  UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
	  // Turn off the serial port, until an address frame arrives
	  // Get interrupts for address frames only
	  UCSR1A = _BV(MPCM1)
	#if defined(PEEL_U2X)
	    | _BV(U2X1)
	#endif
	  ;
}

#endif
