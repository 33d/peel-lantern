#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "circularbuffer.h"
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
#define STATUS_LED _BV(7); // arduino 13
#define CTS _BV(6) // arduino 12
#else
#include "atmegax8.h"
#define STATUS_LED _BV(5); // arduino 13
#define CTS _BV(4) // arduino 12
#endif

#define NUM_TLCS 3
#define LEDS_PER_TLC 16

// 1024x prescaling for the pattern timer
#define PATTERN_CS (_BV(CS12) | _BV(CS10))


// the serial port receive buffer
CircularBuffer<uint8_t, 64> rx_buf;

patternHandler current_pattern_handler = pattern_handlers[0];

#define event GPIOR0
namespace Event {
	const uint8_t update_test_pattern_frame = 2;
}

#define flags GPIOR1
namespace Flags {
	const uint8_t rx_paused = 1;
}

// The coordinates to update next
struct Pos {
	uint8_t row, col;
	Pos() { reset(); }
	void reset() { this->row = 0xFF; this->col = 0xFF; }
} pos;

void die(char* label, uint8_t status) {
	cli();
	printf("Error:%d\n", status);
	while(true) {
		for (uint8_t i = 0; i < status; i++) {
			PORTB |= STATUS_LED;
			_delay_ms(20);
			PORTB &= ~STATUS_LED;
			_delay_ms(100);
		}
		_delay_ms(600);
	}
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

void update_pattern() {
	static uint8_t current_pattern_id = 0;

	// Turn the timer off, to stop the timer interrupt trying to invoke the
	// 0 pointer in the pattern array
	TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));

	uint8_t pattern_id = PATTERN_PIN;
	current_pattern_id = pattern_id;
	uint8_t id = 8;
	for (; pattern_id & 0x80; --id)
		pattern_id <<= 1;
	current_pattern_handler = pattern_handlers[id];

	// Start the timer again if we're in test pattern mode
	if (current_pattern_handler != 0)
		TCCR1B |= PATTERN_CS;
}

ISR(PATTERN_PIN_PCINT_vect) {
	update_pattern();
}

void send_addr(uint8_t addr) {
	// Address mode on
	UCSRB |= _BV(TXB8);
	// Send the address frame for the right slave
	send(addr);
	// Address mode off
	UCSRB &= ~_BV(TXB8);
}

void handle_data(uint8_t data) {
	PORTB &= ~STATUS_LED;
	if (pos.col >= NUM_TLCS * LEDS_PER_TLC * 2) {
		// End of a row, go to the next one
		++(pos.row);
		// Toggle the LED at the end of a frame
		if (pos.row >= 32) {
			pos.row = 0;
		}
		send_addr(pos.row / 8 * 2);
		send(pos.row % 8);
		pos.col = 0;
	} else if (pos.col == NUM_TLCS * LEDS_PER_TLC) {
		// Half row
		send_addr(pos.row / 8 * 2 + 1);
		send(pos.row % 8);
	}

	send(data);
	++(pos.col);
}

ISR(TIMER1_OVF_vect) {
	event |= Event::update_test_pattern_frame;
	// We haven't received any data for a while; reset the pattern coordinates
	pos.reset();
}

void update_test_pattern() {
	// make sure the handler doesn't change during this routine... especially
	// if it changes to 0!
	patternHandler handler = current_pattern_handler;
	if (handler == 0) {
		// timer off
		TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
		PORTB |= STATUS_LED;
		return;
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

			// byte is the TLC pin (ie. one for red, green, blue), col is the led number
			for (uint8_t byte = 0, col = 0; true; byte += 3) {
                for (uint8_t color = 0; color < 3; color++) {
                    send(handler(c, color, col, row));
                    if (byte + color >= NUM_TLCS * LEDS_PER_TLC)
                        goto end;
                }
			}

			end: ;
		}
	}
}

ISR(USART0_RX_vect) {
	// Check for hardware overflow
	if (UCSR0A & _BV(DOR0))
		die(0, 2);
	// Check for software overflow
	if (rx_buf.size() >= rx_buf.capacity() - 8)
		die(0, 3);
	rx_buf.pushBack(UDR0);
	// is the buffer filling up?
	if (!(flags & Flags::rx_paused) && rx_buf.size() > rx_buf.capacity() - 32) {
		PORTB |= CTS; // Tell the computer to stop sending data
		flags |= Flags::rx_paused;
	}

	// Reset the timer.  If we don't get any more data before the timer is
	// up, reset the coordinates.
	TCNT1 = 0;
	TCCR1B |= PATTERN_CS;
}

int main() {
	// status led and CTS
	DDRB = 0xFF;

	peel_serial_init();
	pattern_select_init();

	OCR1A = F_CPU / 1024 / 12;

	TCCR1A = _BV(WGM11) | _BV(WGM10); // Fast PWM, reset at OCR1A
	TCCR1B = _BV(WGM13) | _BV(WGM12) // Fast PWM
					| PATTERN_CS;
	TIMSK1 = _BV(TOIE1); // Interrupt on overflow (=OCR1A)

	// Initialize UART0, which talks to the computer.  Set everything but
	// don't enable tx or rx - the "framebuffer" and "serial" modules will
	// do that.
	UBRR0H = (uint8_t) (UBRR_VAL >> 8);
	UBRR0L = (uint8_t) (UBRR_VAL);
	// Enable TX, RX and interrupt on RX.
	UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);
	// Asynchronous, no parity, 1 stop bit, 8 data bits
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
#if defined(SERIAL_U2X)
	UCSR0A = _BV(U2X0);
#endif


#if defined __AVR_ATmega1280__
	serial_init();
#endif

	update_pattern();

	puts("Ready");

	sei();

	PORTB &= ~CTS; // Ready for data

	while (1) {
		while (rx_buf.size()) {
			handle_data(rx_buf.popFront());
			// can we continue to send data?
			if ((flags & Flags::rx_paused) && rx_buf.size() < 4) {
				flags &= ~Flags::rx_paused;
				PORTB &= ~CTS; // Tell the computer to continue sending data
			}
		}
		if (event & Event::update_test_pattern_frame) {
			update_test_pattern();
			event &= ~Event::update_test_pattern_frame;
		}

		sleep_mode();
	}
}

