#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#define SERIAL_BAUD 500000
#include "serial.h"

#define PEEL_BAUD 500000
// #define PEEL_U2X
#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif
#include "atmega1280.h"

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

ISR(TIMER1_OVF_vect) {
	// Just turn on the UART buffer interrupt, the routine will take care of
	// the rest
	UCSR1B |= _BV(UDRIE1);
}

ISR(USART1_UDRE_vect) {
	PINB |= STATUS_LED;
	static int8_t state = -2;
	static uint8_t lit = 0;

	// Start - blank the display
	if (state == -2) {
		// Enter address mode
		UCSRB |= _BV(TXB8);
		// Send a blank first, to confirm that it works
		UDR = 0xFE;
	} else if (state == -1) {
		// We're still in address mode
		// Send the address frame for slave 0
		UDR = 0;
		// and enter data mode
		UCSRB &= ~_BV(TXB8);
	} else if (state < 12) {
		// Send the data bits
		UDR = data[lit + state];
	} else if (state >= 12) {
		// End of data
		// Don't send another interrupt when the buffer is empty
		UCSR1B &= ~(_BV(UDRIE1));

		// Send the "apply" frame
		UCSRB |= _BV(TXB8);
		UDR = 0xFF;

		++lit;
		if (lit >= 24)
			lit = 0;
		state = -2;
		return;
	}

	++state;
}

int main() {
	DDRB = 0xFF;

	peel_serial_init();

	OCR1A = F_CPU / 1024 / 12;

	TCCR1A = _BV(WGM11) | _BV(WGM10); // Fast PWM, reset at OCR1A
	TCCR1B = _BV(WGM13) | _BV(WGM12) // Fast PWM
					| _BV(CS12) | _BV(CS10); // 1024x prescaling
	TIMSK1 = _BV(TOIE1); // Interrupt on overflow (=OCR1A)

#if defined __AVR_ATmega1280__
	serial_init();
#endif

	puts("Ready");

	sei();

	while (1)
		sleep_mode();
}

