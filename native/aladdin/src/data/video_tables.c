/*
 * video_tables.c: tables the picture code reads.
 */
#include "data/data.h"
#include "data/video_tables.h"
#include "data/data_internal.h"

/* HalfSineBuild reads every other byte from $82:87D4 */
#define QUARTER_SINE_BANK 0x82
#define QUARTER_SINE_ADDR 0x87D4

uint8_t quarter_sine_step(unsigned step)
{
    return cart8(cart_offset(QUARTER_SINE_BANK, QUARTER_SINE_ADDR) + 2u * (step & 0x3F));
}

