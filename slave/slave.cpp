#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

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

ISR(RX_vect) {
	uint8_t addr = UCSRB & _BV(RXB8);
	uint8_t data = UDR;
	printf("Got %x%02x\n", addr, data);
}

int main(void) {

	DDRB = 0xFF;

	serial_init();

	peel_serial_init();

	PORTB |= STATUS_LED;

	puts("Ready\n");

	sei();

	while (1)
		sleep_mode();
}

