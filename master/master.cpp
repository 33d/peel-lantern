#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "patterns.h"

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

#define NUM_TLCS 4

#define STATUS_LED _BV(5); // arduino 13

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
	static uint8_t current_pattern_id = 0;
	static patternHandler current_pattern_handler;

	// Has the pattern changed?  This will be run on the first call, because
	// PINC won't be 0 (unless you fill it with ground wires)
	uint8_t pattern_id = PATTERN_PIN;
	if (pattern_id != current_pattern_id) {
		current_pattern_id = pattern_id;
		uint8_t id = 8;
		for (; pattern_id & 0x80; --id)
			pattern_id <<= 1;
		current_pattern_handler = pattern_handlers[id];
	}

	for (uint8_t c = 0; c < 8; c++) {
		for (uint8_t row = 0; row < 8; row++) {
			// Address mode on
			UCSRB |= _BV(TXB8);
			// Send the address frame for the right slave
			send(c);
			// Address mode off
			UCSRB &= ~_BV(TXB8);
			// followed by the row
			send(row);

			for (uint8_t col = 0; col < NUM_TLCS * 4; col++) {
				for (uint8_t color = 0; color < 3; color++)
					send(current_pattern_handler(c, color, col, row));
			}
		}
	}
}

int main() {
	DDRB = 0xFF;

	// Port C selects the pattern
	PATTERN_DDR = 0;
	// Pull-ups on
	PATTERN_PORT = 0xFF;

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

