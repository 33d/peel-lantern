#include <avr/io.h>
#include <avr/interrupt.h>
#include "framebuffer.h"

Framebuffer fb;

void Framebuffer::init() {
	// Initialize the serial RX.  All other serial initialization isn't up to
	// us.
	// Enable RX, and interrupt on RX
	UCSR0B = _BV(RXEN0) | _BV(RXCIE0);
}

void Framebuffer::handle(uint8_t data) {
	*next = data;
	++next;
	if (next > buf + sizeof(buf))
		next = buf;
}

ISR(USART1_RX_vect) {
	fb.handle(UDR0);
}
