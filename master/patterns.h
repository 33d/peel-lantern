#ifndef PATTERNS_H_
#define PATTERNS_H_

#include <inttypes.h>

typedef uint8_t (*patternHandler)(uint8_t, uint8_t, uint8_t, uint8_t);

uint8_t fading_pattern(uint8_t, uint8_t, uint8_t, uint8_t);
uint8_t fading_vertical_pattern(uint8_t, uint8_t, uint8_t, uint8_t);
uint8_t diagonal_pattern(uint8_t, uint8_t, uint8_t, uint8_t);

const patternHandler pattern_handlers[] = {
		diagonal_pattern, // Pattern shown when port B is disconnected
		diagonal_pattern, // Unavailable on Arduinos
		diagonal_pattern, // Unavailable on Arduinos
		fading_pattern,
		fading_vertical_pattern,
		diagonal_pattern,
		diagonal_pattern,
		diagonal_pattern,
		diagonal_pattern,
};

#endif /* PATTERNS_H_ */
