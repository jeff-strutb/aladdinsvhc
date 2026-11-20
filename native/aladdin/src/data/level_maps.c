/*
 * level_maps.c: the level maps (a shared format, data.h).
 *
 * LevelMapBuild's table at $80:E3DC: per level a word, its header's offset
 * from $E3DC. A header: the screen count, the banks of the block lists, the
 * block tiles and the metatiles, the layers byte (to both screens), then a
 * screen number a screen. A screen's block list is at $8000 + its number *
 * $80 in the block lists' bank; a block word indexes 4 map words at $8000 +
 * the word in the tiles' bank. The layouts' table at $80:E460: per level a
 * word, its entry's offset from $E460: the main layout's offset, then the
 * second's (0: none), each a width less one, a height less one, then the
 * screens row by row. Every sum is the original's, in 16 bits.
 */
#include "data/data.h"
#include "data/data_internal.h"

#define MAPS_BANK 0x80
#define MAPS_ADDR 0xE3DC
#define LAYOUTS_ADDR 0xE460

static uint8_t m8(uint16_t a) { return cart8(cart_offset(MAPS_BANK, a)); }
static uint16_t m16(uint16_t a) { return cart16(cart_offset(MAPS_BANK, a)); }

static uint16_t header(uint16_t level)
{
    return (uint16_t)(MAPS_ADDR + m16((uint16_t)(MAPS_ADDR + 2u * level)));
}

unsigned level_map_screens(uint16_t level)
{
    unsigned n = m8(header(level));

    return n ? n : 256;
}

uint8_t level_map_layers(uint16_t level) { return m8((uint16_t)(header(level) + 4)); }
uint8_t level_map_screen(uint16_t level, unsigned i) { return m8((uint16_t)(header(level) + 5 + i)); }

uint8_t level_screen_list_byte(uint16_t level, uint8_t screen, unsigned i)
{
    uint8_t bank = m8((uint16_t)(header(level) + 1));

    return cart8(cart_offset(bank, (uint16_t)(0x8000u + ((unsigned)screen << 7) + i)));
}

uint16_t level_block_tile(uint16_t level, uint16_t block, unsigned k)
{
    uint8_t bank = m8((uint16_t)(header(level) + 2));

    return cart16(cart_offset(bank, (uint16_t)(0x8000u + block + 2u * k)));
}

DataRef level_metatiles(uint16_t level)
{
    DataRef r;

    r.at = cart_offset(m8((uint16_t)(header(level) + 3)), 0x8000);
    return r;
}

static uint16_t layout_entry(uint16_t level)
{
    return m16((uint16_t)(LAYOUTS_ADDR + 2u * level));
}

static uint16_t layout_at(uint16_t level, unsigned layout)
{
    return m16((uint16_t)(LAYOUTS_ADDR + layout_entry(level) + 2u * layout));
}

unsigned level_map_layout_count(uint16_t level) { return layout_at(level, 1) ? 2 : 1; }

/* ScreenMapLayout counts each down past 0 (DEC, BPL): a byte below $80
   gives it plus one, $80 and up one */
static unsigned counted(uint8_t n) { return n < 0x80 ? n + 1u : 1u; }

unsigned level_map_layout_columns(uint16_t level, unsigned layout)
{
    return counted(m8((uint16_t)(LAYOUTS_ADDR + layout_at(level, layout))));
}

unsigned level_map_layout_rows(uint16_t level, unsigned layout)
{
    return counted(m8((uint16_t)(LAYOUTS_ADDR + layout_at(level, layout) + 1)));
}

uint8_t level_map_layout_screen(uint16_t level, unsigned layout, unsigned i)
{
    return m8((uint16_t)(LAYOUTS_ADDR + layout_at(level, layout) + 2 + i));
}
