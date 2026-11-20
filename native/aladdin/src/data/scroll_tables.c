/*
 * scroll_tables.c: the scroll layers' tables (scroll_tables.h).
 */
#include "data/data.h"
#include "data/data_internal.h"
#include "data/scroll_tables.h"

#define SCROLL_BANK 0x80
#define LAYER_SETUP_ADDR 0xC648
#define EDGE_START_ADDR 0xC656
#define DRAW_STEP_ADDR 0xC666
#define SIDE_EDGE_ADDR 0xC66A
#define SIDE_EDGE_BIT_ADDR 0xC66E

static uint8_t s8(uint16_t a) { return cart8(cart_offset(SCROLL_BANK, a)); }
static uint16_t s16(uint16_t a) { return cart16(cart_offset(SCROLL_BANK, a)); }

LayerSetupEntry layer_setup_entry(unsigned layer)
{
    uint16_t e = (uint16_t)(LAYER_SETUP_ADDR + 7u * layer);
    LayerSetupEntry l;

    l.map_base_high = s8(e);
    l.tilemap_base_high = s8((uint16_t)(e + 1));
    l.tilemap_mask = s16((uint16_t)(e + 2));
    l.screen_flip_high = s8((uint16_t)(e + 4));
    l.column_length = s8((uint16_t)(e + 5));
    l.row_length = s8((uint16_t)(e + 6));
    return l;
}

int16_t edge_start_x(unsigned edge) { return (int16_t)s16((uint16_t)(EDGE_START_ADDR + 4u * edge)); }
int16_t edge_start_y(unsigned edge) { return (int16_t)s16((uint16_t)(EDGE_START_ADDR + 4u * edge + 2)); }
int16_t layer_draw_step(unsigned side) { return (int16_t)s16((uint16_t)(DRAW_STEP_ADDR + 2u * side)); }
uint8_t side_edge(unsigned side) { return (uint8_t)(s8((uint16_t)(SIDE_EDGE_ADDR + side)) / 10u); }
uint8_t side_edge_bit(unsigned side) { return s8((uint16_t)(SIDE_EDGE_BIT_ADDR + side)); }

/* the original reads it as a long address, so past $FFFF it goes on into
   the next bank: the image's next byte */
uint16_t level_metatile_word(DataRef metatiles, uint16_t metatile, unsigned half)
{
    uint32_t at = metatiles.at + ((uint32_t)metatile - 0x8000u) + half;

    return (uint16_t)(cart8(at) | cart8(at + 1) << 8);
}
