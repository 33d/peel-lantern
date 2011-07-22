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

#define PEEL_BAUD 500000
#include "atmega1280.h"

#else
#include "atmegax8.h"
#endif

#define STATUS_LED (_BV(5)) // arduino 13

// The start and end TLC pins to activate
#define TLC_START 1 // inclusive
#define TLC_END 13 // exclusive

// 0xFF = idle, 0-(NUM_TLCS*24) = receiving data
volatile uint8_t state = 0xFF;
uint8_t id = 0;

uint8_t *tlc_data = tlc_GSData;

volatile uint8_t message_sent = 0;

void show_data() {
	for (uint8_t i = 0; i < NUM_TLCS * 24; i++)
		printf("%02x", tlc_data[i]);
	printf("\n");
	message_sent = 0;
}

void show_error(uint8_t error) {
	printf("Error %d\n", error);
	Tlc.set(error + 1, 4095);
	sei();
	sleep_mode();
}

void blank() {
	Tlc.clear();
}

void apply() {
	message_sent = 1;
	Tlc.update();
}

ISR(RX_vect) {
	uint8_t isAddr = UCSRB & _BV(RXB8);
	uint8_t data = UDR;

	if (isAddr) {
		if (data == id) {
			state = TLC_START;
			// Turn on interrupts for data frames
			UCSRA &= ~(_BV(MPCM));
		} else if (data == 0xFF) {
			if (state != 0xFF)
				show_error(1);
			apply();
		} else if (data == 0xFE) {
			if (state != 0xFF)
				show_error(2);
			blank();
		}
	} else {
		if (state < TLC_END) {
			Tlc.set(state++, ((uint16_t) data) << 4);
		}
		if (state >= TLC_END) {
			// End of data, go back to address mode
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

	while (1) {
		sleep_mode();
		// Run the serial code here, so interrupts can still run
		if (message_sent)
			show_data();
	}
}

