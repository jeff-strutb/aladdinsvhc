/*
 * parallax_tables.c: the tables the parallax reads.
 */
#include "data/data.h"
#include "data/data_internal.h"
#include "data/parallax_tables.h"

#define BANDS_BANK 0x80
#define BAND_COUNTS 0xC602
#define BAND_EDGES 0xC606
#define BAND_TABLES 0xC628

uint16_t band_set_count2(uint16_t set) { return cart16(cart_offset(BANDS_BANK, (uint16_t)(BAND_COUNTS + set))); }
uint16_t band_edge_word(uint16_t at) { return cart16(cart_offset(BANDS_BANK, (uint16_t)(BAND_EDGES + at))); }
uint16_t band_table_place(uint16_t buffer) { return cart16(cart_offset(BANDS_BANK, (uint16_t)(BAND_TABLES + buffer))); }
