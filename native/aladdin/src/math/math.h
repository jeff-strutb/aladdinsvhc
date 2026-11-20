/*
 * math.h: the game's arithmetic that the original did with the CPU's
 * multiplier and divider, as ordinary C.
 */
#ifndef ALADDIN_MATH_H
#define ALADDIN_MATH_H

#include <stdint.h>

/*
 * @routine 81:A7E6 MultiplyBy
 * WHAT IT DOES: The 16-bit product of two bytes: the factor the caller left
 * in the multiplier times A (the original's Y on return).
 */
static inline uint16_t multiply_bytes(uint8_t a, uint8_t b)
{
    return (uint16_t)(a * b);
}

#endif
