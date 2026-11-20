/*
 * data_internal.h: data/'s own access to cart.dat. Not for the rest of the game.
 */
#ifndef ALADDIN_DATA_INTERNAL_H
#define ALADDIN_DATA_INTERNAL_H

#include <stdint.h>

uint32_t cart_offset(uint8_t bank, uint16_t addr);  /* the image offset of the original's bank and address */
const uint8_t *cart_at(uint32_t offset);
uint8_t cart8(uint32_t offset);
uint16_t cart16(uint32_t offset);

#endif
