#if !defined(__PEEL_ATMEGA_X8_H__)
#define __PEEL_ATMEGA_X8_H__

#include <stdio.h>

#if defined __AVR_ATmega1280__
#define RX_vect USART_RX0_vect
#else
#define RX_vect USART_RX_vect
#endif

#define UDR UDR0
#define UCSRA UCSR0A
#define RXB8 RXB80
#define DOR DOR0
// Enable RX
#define enable_serial() (UCSR0B |= _BV(RXEN0))

#if !defined(PEEL_BAUD)
#error Define PEEL_BAUD first
#endif

#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

void peel_serial_init() {
  // Init serial
  UBRR0H = (uint8_t) (PEEL_UBRR_VAL >> 8);
  UBRR0L = (uint8_t) (PEEL_UBRR_VAL);
  // Turn on the RX interrupt
  UCSR0B = _BV(RXCIE0);
  // Asynchronous, no parity, 1 stop bit, 8 data bits (the high bit is the
  // address bit)
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  UCSR0A = 0
#if defined(SERIAL_U2X)
    | _BV(U2X0)
#endif
  ;
}

#endif
