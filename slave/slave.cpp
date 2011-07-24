#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#include "Tlc5940.h"

#define PEEL_BAUD 500000

#if defined __AVR_ATmega1280__
// Configure the serial port for debugging
#define SERIAL_BAUD 500000
//#define SERIAL_U2X 0
#include "serial.h"

#include "atmega1280.h"

#else
#include "atmegax8.h"
#endif

#define STATUS_LED (_BV(5)) // arduino 13

// The start and end TLC pins to activate
#define TLC_START 1 // inclusive
#define TLC_END 13 // exclusive

// 0xFF = idle, 0-(NUM_TLCS*16) = receiving data
// 0..NUM_TLCS * 16 is the index of the pin being written to.  Since the TLC
// has 16 outputs, the top 4 bits of this number contain the index of the chip
// currently being written to.
volatile uint8_t state = 0xFF;
volatile uint8_t id;

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
		// Do we still have more data for this chip?  The bottom 4 bits tell
		// us the pin for the current chip.
		if ((state & 0x0F) < TLC_END) {
			Tlc.set(state++, ((uint16_t) data) << 4);
		}
		if ((state & 0x0F) >= TLC_END) {
			// Set state to pin 0 of the next chip
			state = (state & 0xF0) + 16;
			// Have we run out of chips to write to?
			if (state >= NUM_TLCS * 16) {
				// End of data, go back to address mode
				state = 0xFF;
				UCSRA |= _BV(MPCM);
		    } else {
		    	// go to the next chip
		    	state += TLC_START;
		    }
		}
	}
}

int main(void) {

	DDRB = 0xFF;

#if defined __AVR_ATmega1280__
	serial_init();
#endif

	peel_serial_init();

	PORTB |= STATUS_LED;

	// What's my address?  It's stored in address 0 of the EEPROM.
	EEAR = 0;
	EECR |= _BV(EERE);
	id = EEDR;

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

