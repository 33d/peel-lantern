#if !defined(__PEEL_ATMEGA_X8_H__)
#define __PEEL_ATMEGA_X8_H__

#define RX_vect USART_RX_vect
#define UDR UDR0
#define UCSRA UCSR0A
#define UCSRB UCSR0B
#define RXB8 RXB80
#define MPCM MPCM0

#if !defined(PEEL_BAUD)
#error Define PEEL_BAUD first
#endif

// Debugging is unavailable; we're using the only serial port
#define printf(...) ;
#define puts(x) ;

#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / SERIAL_BAUD) - 1)
#endif

void peel_serial_init() {
  // Init serial
  UBRR0H = (uint8_t) (UBRR_VAL >> 8);
  UBRR0L = (uint8_t) (UBRR_VAL);
  // rx, enable interrupt for rx
  UCSR0B = _BV(RXEN0) | _BV(RXCIE0)
  // Asynchronous, no parity, 1 stop bit, 9 data bits (9th bit is the address
  // bit for multi-processor mode)
      | _BV(UCSZ02);
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  // Turn off the serial port, until an address frame arrives
  // Get interrupts for address frames only
  UCSR0A = _BV(MPCM0)
#if defined(SERIAL_U2X)
    | _BV(U2X0)
#endif
  ;
}

#endif
