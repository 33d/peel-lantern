#include <string.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#define PEEL_BAUD 300
// #define PEEL_U2X
#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

#define STATUS_LED _BV(5); // arduino 13
void die(char* label, uint8_t status) {
	cli();
	PORTB |= STATUS_LED;
	sleep_cpu();
}

ISR(TIMER1_OVF_vect) {
	static uint8_t val = 0;

	PINB |= STATUS_LED;

	// wait until transmit buffer is empty
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = val++;
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
	// Asynchronous, no parity, 1 stop bit, 8 data bits
	/*| _BV(UCSZ02)*/;
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
#if defined(SERIAL_U2X)
	UCSR0A = _BV(U2X0);
#endif

	sei();

	while (1)
		sleep_mode();
}

