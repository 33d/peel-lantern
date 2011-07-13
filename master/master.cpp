#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#define PEEL_BAUD 500000
// #define PEEL_U2X
#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

#define STATUS_LED _BV(5); // arduino 13

// Some fairly random test data.  I have no idea what this will do...
const uint8_t sample_data[] = {
		0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
		0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
		0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00
};

void die(char* label, uint8_t status) {
	cli();
	PORTB |= STATUS_LED;
	sleep_cpu();
}

void send(uint8_t c) {
	UDR0 = c;
	// wait until transmit buffer is empty.  The datasheet says to do this
	// BEFORE you send the data, but I want to immediately change the
	// address bit, so it makes more sense here to do it after.  (Changing
	// the address bit before the data is sent seems to affect the
	// transmission.)
	while (!(UCSR0A & _BV(UDRE0)));
}

ISR(TIMER1_OVF_vect) {
	PINB |= STATUS_LED;

	// Enter address mode
	UCSR0B |= _BV(TXB80);
	// Send a blank first, to confirm that it works
	send(0xFE);
	// Send the address frame for slave 0
	send(0);
	UCSR0B &= ~_BV(TXB80);

	for (uint8_t i = 0; i < 24; i++)
		send(sample_data[i]);

	// Send the apply frame
	UCSR0B |= _BV(TXB80);
	send(0xFF);
	UCSR0B &= ~_BV(TXB80);
}

int main() {
	OCR1A = F_CPU / 1024;

	TCCR1A = _BV(WGM11) | _BV(WGM10); // Fast PWM, reset at OCR1A
	TCCR1B = _BV(WGM13) | _BV(WGM12) // Fast PWM
			| _BV(CS12) | _BV(CS10); // 1024x prescaling
	TIMSK1 = _BV(TOIE1); // Interrupt on overflow (=OCR1A)

	DDRB |= STATUS_LED;

	// Init serial
	UBRR0H = (uint8_t)(PEEL_UBRR_VAL >> 8);
	UBRR0L = (uint8_t)(PEEL_UBRR_VAL);
	// transmitter only
	UCSR0B = _BV(TXEN0)
	// Asynchronous, no parity, 1 stop bit, 9 data bits
		| _BV(UCSZ02);
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
#if defined(SERIAL_U2X)
	UCSR0A = _BV(U2X0);
#endif

	sei();

	while (1)
		sleep_mode();
}

