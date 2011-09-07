#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <math.h>

#include "Tlc5940.h"

#define PEEL_BAUD 1000000
#define SERIAL_BAUD PEEL_BAUD
// Configure the serial port for debugging
#include "serial.h"

#if defined __AVR_ATmega1280__
#include "atmega1280.h"
#define STATUS_LED (_BV(7)) // arduino 13
#else
#include "atmegax8.h"
#define STATUS_LED (_BV(5)) // arduino 13
#endif

// The start and end TLC pins to activate
#define TLC_START 1 // inclusive
#define TLC_END 13 // exclusive

#define ROWS 8

// Calls die_impl.  Using asm stops the compiler trying to save
// registers at the start of interrupt routines, considerably reducing
// the ISR prologue.
#define die(x) __asm__("ldi r24, %0 \n\t jmp die_impl \n\t" : : "M" (x))

uint8_t id;

uint8_t tlc_data[NUM_TLCS * 24 * ROWS];
// The data that the TLC library uses.  I've removed it from Tlc5940.cpp; this
// one will point somewhere into tlc_data depending on the row.
uint8_t* tlc_GSData = tlc_data;

// The lookup table for the LED brightness.  If we need the RAM space, this
// could be pregenerated, and stored in flash.
uint16_t lookup_even[128]; // for even columns - the bits are shifted
uint16_t lookup_odd[128];  // for odd columns
// The magic number that adjusts the brightness - higher numbers make
// mid-range values darker
#define BRIGHTNESS 3

// Events
// Keep the events in a low register, for fast access
// I used to use GPIOR0, but now I use that for the data.  Port C will do,
// but remember that bit 7 is unavailable, and bits 0 and 3 are used for the
// shift register.
#define events PORTC
namespace Event {
	static const uint8_t update_row = _BV(1);
	static const uint8_t message_sent = _BV(2);
	static const uint8_t rx_valid = _BV(4);
};
// Received data will go here for processing by the main loop.
#define rx_data GPIOR0

// 0xFF = idle, 0xFE=waiting for which row to update,
// 0-(NUM_TLCS*16) = receiving data
// 0..NUM_TLCS * 16 is the index of the pin being written to.  Since the TLC
// has 16 outputs, the top 4 bits of this number contain the index of the chip
// currently being written to.
static uint8_t state = 0xFF;

uint8_t row = 0;

void updateRow() {
	++row;
	if (row >= ROWS)
		row = 0;
	tlc_GSData = tlc_data + row * NUM_TLCS * 24;
	Tlc.update();
}

// Called just after the data is committed to the output pins
// The "inline" reduces the ISR prologue
inline void tlc_onUpdateFinished() {
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

ISR(TIMER1_OVF_vect) {
    disable_XLAT_pulses();
    clear_XLAT_interrupt();
    tlc_needXLAT = 0;
	tlc_onUpdateFinished();
}

void show_data() {
	events &= ~Event::message_sent;
	for (uint8_t i = 0; i < NUM_TLCS * 24; i++)
		printf("%02x", tlc_data[i]);
	printf("\n");
}

// I need to reference die_impl from assembler, C++ will mangle the name
extern "C" {

// This was supposed to get rid of the ISR prologue, but it doesn't seem to
// work
void die_impl(uint8_t) __attribute__ ((noreturn));

void die_impl(uint8_t status) {
	cli();
	// Turn off SPI, so we can control pin 13
	SPCR &= ~_BV(SPE);
	DDRB |= STATUS_LED;
	printf("Error:%02x\n", status);
	for (uint8_t j = 0; j < 10; j++) {
		for (uint8_t i = 0; i < status; i++) {
			PORTB |= STATUS_LED;
			_delay_ms(20);
			PORTB &= ~STATUS_LED;
			_delay_ms(200);
		}
		_delay_ms(600);
	}
	// Try a reset... will this work?
	__asm__("jmp 0");
	// stop the compiler complaining that this function returns
	abort();
}

}

// Points to the start of the row being updated
volatile uint8_t* row_start = tlc_data;

void handle_addr(uint8_t data) {
	// Check the state - whinge if we don't have enough data for a row
	if (state != 0xFF) {
		die(6);
	}
	// The low 4 bits contain the chip ID... is this us?
	if ((data & 0x0F) == id) {
		// The row number is in bits 4-6
		uint8_t row_num = (data >> 4) & 0x07;
		row_start = tlc_data + (NUM_TLCS * 24 * row_num);
		state = TLC_START;
	}
}

void handle_data(uint8_t data) {
	// The next part of the "framebuffer" to write to
	static uint8_t* buf;

	// Do we still have more data for this chip?  The bottom 4 bits tell
	// us the pin for the current chip.
	if ((state & 0x0F) < TLC_END) {
		// Update the framebuffer
		if (state & 1) {
			// odd column
			uint16_t odd = lookup_odd[data];
			*buf |= odd >> 8;
			++buf;
			*buf = (uint8_t) odd;
			++buf;
		} else {
			uint16_t val = lookup_even[data];
			*((uint16_t*) buf) = val;
			++buf;
		}
		++state;
	}
	if ((state & 0x0F) >= TLC_END) {
		// Set state to pin 0 of the next chip
		state = (state & 0xF0) + 16;
		// Have we run out of chips to write to?
		if (state >= NUM_TLCS * 16) {
			// End of data, go back to address mode
			state = 0xFF;
		} else {
			// go to the next chip
			state += TLC_START;
			buf += (16 - TLC_END + TLC_START) * 3 / 2;
		}
	}
}

// Don't call any functions from this method, because it makes a huge prologue
ISR(RX_vect) {
	// Forcing r0 to be used saves a push instruction
	volatile register uint8_t r0 asm("r0");

	// Check for hardware buffer overflow - force the compiler to use r0
	// instead of picking one at random
	if ((r0 = UCSRA) & _BV(DOR))
		// This function never returns, so we don't care which registers
		// it clobbers
		die(2);

	// Make sure the previous message was handled
	if (events & Event::rx_valid)
		die(3);

	rx_data = (r0 = UDR);
	events |= Event::rx_valid;
}

void handle_rx_data(uint8_t data) {
	uint8_t isAddr = data & 0x80;

	if (isAddr) {
		handle_addr(data);
	} else if (state != 0xFF) {
		handle_data(data);
	}
}

void generate_lookup() {
	// The lookup value is for the formula y = nx^a, where x is the incoming
	// pixel value, y is the LED value, and a is BRIGHTNESS.
	float n = 4096.0 / pow(256, BRIGHTNESS);
	uint8_t i = 0;
	// don't use "for" with the 8-bit value!
	do {
		lookup_odd[i] = n * pow(i*2, BRIGHTNESS);
		lookup_even[i] = lookup_odd[i] << 4;
	} while (++i < 128);
}

int main(void) {
    generate_lookup();

	// Shift register (I actually clobber all of port C)
	DDRC = 0xFF;
	// I abuse port C to put the event flags in, so make sure nobody else
	// can change them
	DDRC = 0xFF;

	peel_serial_init();
	serial_init();

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

	// Wait until here to enable the serial port, so data already on the
	// line doesn't overflow the rx buffer
	enable_serial();

	while (1) {
		while (events) {
			// RX events get top priority
			if (events & Event::rx_valid) {
				// grab a copy of rx_data first, so the interrupt doesn't
				// change it on us
				uint8_t data = rx_data;
				events &= ~Event::rx_valid;
				handle_rx_data(data);
			}
			// Run the serial code here, so interrupts can still run
			else if (events & Event::message_sent)
				show_data();
			else if (events & Event::update_row) {
				events &= ~Event::update_row;
				updateRow();
			}
		}
		sleep_mode();
	}

	return 0;
}

