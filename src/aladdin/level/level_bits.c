/*
 * level_bits.c: the eight bytes at $7E:F260, set at a level's start from
 * the bits of a table byte picked by $035D.
 */
#include "machine.h"
#include "ops65.h"

/* Sets the byte at $7E:F260 + X from bit X of the table byte by $035D. */
#define BIT_TO_BYTE 0x83C2F0u








