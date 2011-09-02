#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <math.h>

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

#define ROWS 8

uint8_t id;

uint8_t tlc_data[NUM_TLCS * 24 * ROWS];
// The data that the TLC library uses.  I've removed it from Tlc5940.cpp; this
// one will point somewhere into tlc_data depending on the row.
uint8_t* tlc_GSData = tlc_data;

// The lookup table for the LED brightness.  If we need the RAM space, this
// could be pregenerated, and stored in flash.
uint16_t lookup[128];
// The magic number that adjusts the brightness - higher numbers make
// mid-range values darker
#define BRIGHTNESS 3

// Events
// Keep the events in a low register, for fast access
#define events GPIOR0
namespace Event {
	static const uint8_t update_row = 0x1;
	static const uint8_t message_sent = 0x2;
};

uint8_t row = 0;

void updateRow() {
	++row;
	if (row >= ROWS)
		row = 0;
	tlc_GSData = tlc_data + row * NUM_TLCS * 24;
	Tlc.update();
}

// Called just after the data is committed to the output pins
void tlc_onUpdateFinished() {
	// Update the shift register ASAP after the XLAT pulse
	if (row == 0)
		// Clock in 1 bit of data
		PORTC &= ~_BV(PORTC0);
	__asm__("nop\n\t");
	PORTC |= _BV(PORTC3);
	PORTC |= _BV(PORTC0);
	PORTC &= ~_BV(PORTC3);

	events |= Event::update_row;
}

void show_data() {
	events &= ~Event::message_sent;
	for (uint8_t i = 0; i < NUM_TLCS * 24; i++)
		printf("%02x", tlc_data[i]);
	printf("\n");
}

void die(uint8_t status) {
	cli();
	for (uint8_t i = 0; i < 10; i++) {
		for (uint8_t i = 0; i < status; i++) {
			PORTB |= STATUS_LED;
			_delay_ms(20);
			PORTB &= ~STATUS_LED;
			_delay_ms(100);
		}
		_delay_ms(600);
	}
	// Try a reset... will this work?
	__asm__("jmp 0");
}

ISR(RX_vect) {
	// 0xFF = idle, 0xFE=waiting for which row to update,
	// 0-(NUM_TLCS*16) = receiving data
	// 0..NUM_TLCS * 16 is the index of the pin being written to.  Since the TLC
	// has 16 outputs, the top 4 bits of this number contain the index of the chip
	// currently being written to.
	static uint8_t state = 0xFF;
	static uint8_t* row_start = tlc_data;

	uint8_t data = UDR;
	uint8_t isAddr = UDR & 0x8F;

	if (isAddr) {
		// Check the state - whinge if we don't have enough data for a row
		if (state != 0xFF) {
			die(6);
			return;
		}
		if (data == id) {
			state = 0xFE; // wait for the row number
			// Turn on interrupts for data frames
			UCSRA &= ~(_BV(MPCM));
		}
	} else {
		if (state == 0xFE) {
			row_start = tlc_data + (NUM_TLCS * 24 * data);
			state = TLC_START;
		} else {
			// Do we still have more data for this chip?  The bottom 4 bits tell
			// us the pin for the current chip.
			if ((state & 0x0F) < TLC_END) {
				// Be sure to update the correct part of the framebuffer
				Tlc.set(state++, lookup[data], row_start);
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
}

void generate_lookup() {
	// The lookup value is for the formula y = nx^a, where x is the incoming
	// pixel value, y is the LED value, and a is BRIGHTNESS.
	float n = 4096.0 / pow(256, BRIGHTNESS);
	uint8_t i = 0;
	// don't use "for" with the 8-bit value!
	do {
		lookup[i] = n * pow(i*2, BRIGHTNESS);
	} while (++i < 128);
}

int main(void) {
    generate_lookup();

	// Shift register (I actually clobber all of port C)
	DDRC = 0xFF;

#if defined __AVR_ATmega1280__
	serial_init();
#endif

	peel_serial_init();

	// What's my address?  It's stored in address 0 of the EEPROM.
	EEAR = 0;
	EECR |= _BV(EERE);
	id = EEDR;

	puts("Ready");

	// Start the TLC on a fairly dark colour
	Tlc.init(0xFF);
	// Light up some of the first row, to show the chip ID
	for (uint8_t i = 0; i < id*3; i++)
		tlc_data[i] = (i % 3 == 2) ? 0xFF : 0;

	sei();

	// Start sending data to the TLC
	enable_XLAT_pulses();
	set_XLAT_interrupt();

	while (1) {
		while (events) {
			// Run the serial code here, so interrupts can still run
			if (events & Event::message_sent)
				show_data();
			if (events & Event::update_row) {
				events &= ~Event::update_row;
				updateRow();
			}
		}
		sleep_mode();
	}

	return 0;
}

