#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#include "Tlc5940.h"

#if defined __AVR_ATmega1280__
// Configure the serial port for debugging
#define SERIAL_BAUD 500000
//#define SERIAL_U2X 0
#include "serial.h"

#define PEEL_BAUD 300
#include "atmega1280.h"

#else
#include "atmegax8.h"
#endif

#define STATUS_LED (_BV(5)) // arduino 13

// 0xFF = idle, 0-(NUM_TLCS*24) = receiving data
uint8_t state = 0xFF;
uint8_t id = 0;

uint8_t *tlc_data = tlc_GSData;

void blank() {
	Tlc.clear();
	puts("Blank");
}

void apply() {
	printf("Apply: ");
	for (uint8_t i = 0; i < NUM_TLCS * 24; i++)
		printf("%02X", tlc_data[i]);
	printf("\n");
	Tlc.update();
}

ISR(RX_vect) {
	uint8_t isAddr = UCSRB & _BV(RXB8);
	uint8_t data = UDR;

	if (isAddr) {
		if (data == id) {
			printf("Got address frame, data: ");
			state = 0;
			// Turn on interrupts for data frames
			UCSRA &= ~(_BV(MPCM));
		} else if (data == 0xFF) {
			apply();
		} else if (data == 0xFE) {
			blank();
		}
	} else {
		if (state < NUM_TLCS * 24) {
			printf("%02x", data);
			tlc_data[state++] = data;
		}
		if (state >= NUM_TLCS * 24) {
			// End of data, go back to address mode
			puts(", end");
			state = 0xFF;
			UCSRA |= _BV(MPCM);
		}
	}
}

int main(void) {

	DDRB = 0xFF;

	serial_init();

	peel_serial_init();

	PORTB |= STATUS_LED;

	puts("Ready");

	// Start the TLC on a fairly dark colour
	Tlc.init(0xFF);

	sei();

	while (1)
		sleep_mode();
}

