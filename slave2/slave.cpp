#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define PEEL_BAUD 2000000
#define PEEL_U2X
// #define PEEL_U2X
#define STATUS_LED (_BV(5)) // arduino 13
// Enable RX
#define enable_serial() (UCSR0B |= _BV(RXEN0))
// Set OC1B at bottom, clear at compare match
#define enable_blank() (TCCR1A |= _BV(COM1B1))
#define disable_blank() (TCCR1A &= ~_BV(COM1B1))

#define id SLAVE_ID

#if !defined(PEEL_BAUD)
#error Define PEEL_BAUD first
#endif

#if defined(PEEL_U2X)
#define PEEL_UBRR_VAL ((F_CPU / 8 / PEEL_BAUD) - 1)
#else
#define PEEL_UBRR_VAL ((F_CPU / 16 / PEEL_BAUD) - 1)
#endif

#define XLAT_PORT PORTB
#define XLAT_DDR  DDRB
#define XLAT_BIT  PB1
#define BLANK_PORT PORTB
#define BLANK_DDR  DDRB
#define BLANK_BIT  PB2
#define SCK_DDR  DDRB
#define SCK_BIT  PB3
#define SIN_DDR  DDRB
#define SIN_BIT  PB5
#define GSCLK_DDR  DDRD
#define GSCLK_BIT  PD3
#define SS_DDR DDRB
#define SS_BIT PB2
#define SHIFT_CLOCK_PORT PORTC
#define SHIFT_CLOCK_DDR  DDRC
#define SHIFT_CLOCK_BIT  PC3
#define SHIFT_DATA_PORT PORTC
#define SHIFT_DATA_DDR  DDRC
#define SHIFT_DATA_BIT  PC0

// Events
// Keep the events in a low register, for fast access
// I used to use GPIOR0, but now I use that for the data.  Port C will do,
// but remember that bit 7 is unavailable, and bits 0 and 3 are used for the
// shift register.
#define events PORTC
namespace Event {
	static const uint8_t receiving_data = _BV(1);
};

// How many bytes of data we've received for this row
#define rx_count GPIOR0

// the most recent address byte we've received
uint8_t addr_byte;

void die(uint8_t) __attribute__ ((noreturn));
void die(uint8_t status) {
	cli();
	// Turn off SPI, so we can control pin 13
	SPCR &= ~_BV(SPE);
	DDRB |= STATUS_LED;
	// wait for the serial output buffer to clear
	while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = '0' + status;
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

static void update_row(uint8_t addr_byte) {
	uint8_t row = addr_byte & 0x0E;
	// "row" is shifted left one, so double everything (this seems to be much
	// faster than performing a shift on "row")
	for (uint8_t i = 0; i < 16; i += 2) {
		if (i == row) {
			// The datasheet says that a 20ns pulse is adequate.  One
			// instruction is 62ns, so I don't think we need any delays.
			SHIFT_DATA_PORT &= ~_BV(SHIFT_DATA_BIT);
			SHIFT_CLOCK_PORT |= _BV(SHIFT_CLOCK_BIT);
			SHIFT_DATA_PORT |= _BV(SHIFT_DATA_BIT);
			SHIFT_CLOCK_PORT &= ~_BV(SHIFT_CLOCK_BIT);
		} else {
			SHIFT_CLOCK_PORT |= _BV(SHIFT_CLOCK_BIT);
			SHIFT_CLOCK_PORT &= ~_BV(SHIFT_CLOCK_BIT);
		}
	}
}

static uint8_t read_data() {
	// Check for hardware buffer overflow
	if (UCSR0A & _BV(DOR0))
		// This function never returns, so we don't care which registers
		// it clobbers
		die(2);

	return UDR0;
}

// Throws away any data in the receive buffer, to clear the data overrun flag
static void clear_rx() {
	uint8_t temp;
	while (UCSR0A & _BV(RXC0))
		temp = UDR0;
}

// the "static" seems to inline this function
static void handle_data(uint8_t data) {
	// Address byte
	if (data & 1) {
		// The top 4 bits of "data" contain the address... is this for us?
		if (((data ^ (id << 4)) & 0xF0) == 0) {
			addr_byte = data; // save the row, which is in this byte
			rx_count = 0;
			events |= Event::receiving_data;
		} else
			events &= ~Event::receiving_data;
	} else if (events & Event::receiving_data) {
		// Check that the previous byte was sent
		if (SPSR & _BV(WCOL))
			die(4);
		SPDR = data;
		++rx_count;
		// is that it?
		if (rx_count >= 96) {
			// wait until the last bit of data has been sent
			while (!(SPSR & _BV(SPIF)));
			// stop the BLANK line
			disable_blank();
			// and raise it
			BLANK_PORT |= _BV(BLANK_BIT);
			XLAT_PORT |= _BV(XLAT_BIT);
			update_row(addr_byte);
			events &= ~Event::receiving_data;
			XLAT_PORT &= ~_BV(XLAT_BIT);
			BLANK_PORT &= ~_BV(BLANK_BIT);
			enable_blank();
			// Loading the shift register takes a while, and it might have
			// overflowed.  Get rid of any data and clear the error.
			clear_rx();
		}
	}
}

void init_blank_timer() {
	// BLANK is an output; the timer output won't work without this
	BLANK_DDR |= _BV(BLANK_BIT);

	// Fast PWM mode, TOP=ICR1
	TCCR1A = _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12);
	// Blank every 4096 GSCLK cycles.  That runs at F_CPU/2.
	ICR1 = 8192;
	// Clear BLANK when timer=1
	OCR1B = 1;
	// Timer on
	// No prescaling (timer runs at clock frequency), keeps the BLANK
	// signal short
	TCCR1B |= _BV(CS10);
}

void init_gsclk() {
	GSCLK_DDR |= _BV(GSCLK_BIT);

	// Set GSCLK (OC2B) when TCNT2=1, clear on overflow.
	TCCR2A = _BV(COM2B1) | _BV(COM2B0)
	// Fast PWM mode, TOP=OCR2A.
		| _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(WGM22);
	// Set TOP
	OCR2A = 1;
	// Start timer, no prescaling
	TCCR2B |= _BV(CS20);
}

void init_tlc_data() {
	// Set MOSI and SCK to output.  I don't know if I need this, but it won't
	// hurt.
	SCK_DDR |= _BV(SCK_BIT);
	SIN_DDR |= _BV(SIN_BIT);
	// Set the SS pin as an output, because the chip tries fancy stuff if it's
	// an input.  This is the same as BLANK, but let's not be too careful.
	SS_DDR |= _BV(SS_BIT);

	// Enable SPI, Master, set clock rate fck/2
	SPCR = _BV(SPE) | _BV(MSTR);
	SPSR = _BV(SPI2X);
}

void init_xlat() {
	XLAT_DDR |= _BV(XLAT_BIT);
	XLAT_PORT &= ~_BV(XLAT_BIT);
}

void init_serial() {
  // Init serial
  UBRR0H = (uint8_t) (PEEL_UBRR_VAL >> 8);
  UBRR0L = (uint8_t) (PEEL_UBRR_VAL);
  // Asynchronous, no parity, 1 stop bit, 8 data bits
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  // TX enable. Don't turn on RX yet - wait until we're ready to receive
  UCSR0B = _BV(TXEN0);
  UCSR0A = 0
#if defined(PEEL_U2X)
	| _BV(U2X0)
#endif
  ;
}

void init_shift_register() {
	SHIFT_CLOCK_DDR |= _BV(SHIFT_CLOCK_BIT);
	SHIFT_DATA_DDR |= _BV(SHIFT_DATA_BIT);
	// leave the data line high
	SHIFT_DATA_PORT |= _BV(SHIFT_DATA_BIT);
}

int main(void) {
	init_tlc_data();
	init_serial();
	init_blank_timer();
	init_gsclk();
	init_xlat();
	init_shift_register();

	// Send a test message
	UDR0 = id + 'a';

	// Wait until here to enable the serial port, so data already on the
	// line doesn't overflow the rx buffer
	enable_serial();

	while(true) {
		// is there serial data?
		if (UCSR0A & _BV(RXC0))
			handle_data(read_data());
	}

	return 0;
}
