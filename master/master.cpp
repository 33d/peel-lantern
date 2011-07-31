#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

// Settings for the computer-side serial port
#define SERIAL_BAUD 500000
#if defined(SERIAL_U2X)
#define UBRR_VAL ((F_CPU / 8 / SERIAL_BAUD) - 1)
#else
#define UBRR_VAL ((F_CPU / 16 / SERIAL_BAUD) - 1)
#endif
#include "serial.h"

// Settings for the slave-side serial port
#define PEEL_BAUD 500000
// #define PEEL_U2X
#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

#if defined __AVR_ATmega1280__
#include "atmega1280.h"
#else
#include "atmegax8.h"
#endif

#define STATUS_LED _BV(5); // arduino 13

const uint8_t data[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0x44, 0x88, 0xBB, 0xFF, 0xBB, 0x88, 0x44, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void die(char* label, uint8_t status) {
	cli();
	PORTB |= STATUS_LED;
	sleep_cpu();
}

void send(uint8_t c) {
	UDR = c;
	// wait until transmit buffer is empty.  The datasheet says to do this
	// BEFORE you send the data, but I want to immediately change the
	// address bit, so it makes more sense here to do it after.  (Changing
	// the address bit before the data is sent seems to affect the
	// transmission.)
	while (!(UCSRA & _BV(UDRE)));
}

ISR(TIMER1_OVF_vect) {
	PINB |= STATUS_LED;
	static uint8_t lit = 0;

	// Enter address mode
	UCSRB |= _BV(TXB8);
	// Send the address frame for slave 0
	send(0);
	UCSRB &= ~_BV(TXB8);

	for (uint8_t i = lit; i < lit + 12; i++)
		send(data[i]);
	++lit;
	if (lit >= 24)
		lit = 0;
}

int main() {
	DDRB = 0xFF;

	peel_serial_init();

	OCR1A = F_CPU / 1024 / 12;

	TCCR1A = _BV(WGM11) | _BV(WGM10); // Fast PWM, reset at OCR1A
	TCCR1B = _BV(WGM13) | _BV(WGM12) // Fast PWM
					| _BV(CS12) | _BV(CS10); // 1024x prescaling
	TIMSK1 = _BV(TOIE1); // Interrupt on overflow (=OCR1A)

	// Initialize UART0, which talks to the computer.  Set everything but
	// don't enable tx or rx - the "framebuffer" and "serial" modules will
	// do that.
	UBRR0H = (uint8_t) (UBRR_VAL >> 8);
	UBRR0L = (uint8_t) (UBRR_VAL);
	// Asynchronous, no parity, 1 stop bit, 8 data bits
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
#if defined(SERIAL_U2X)
	UCSR0A = _BV(U2X0);
#endif


#if defined __AVR_ATmega1280__
	serial_init();
#endif

	puts("Ready");

	sei();

	while (1)
		sleep_mode();
}

