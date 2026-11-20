/*
 * input_tables.c: the demos' pad scripts at $84:F700, two bytes an entry:
 * pad 1's high byte, then its frame count.
 */
#include "data/data.h"
#include "data/data_internal.h"

#define DEMO_SCRIPTS_BANK 0x84
#define DEMO_SCRIPTS_ADDR 0xF700

DemoStep demo_step_at(uint16_t at)
{
    uint32_t o = cart_offset(DEMO_SCRIPTS_BANK, (uint16_t)(DEMO_SCRIPTS_ADDR + at));
    DemoStep s;

    s.buttons = cart8(o);
    s.frames = cart8(cart_offset(DEMO_SCRIPTS_BANK, (uint16_t)(DEMO_SCRIPTS_ADDR + at + 1)));
    return s;
}
