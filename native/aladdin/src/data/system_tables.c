/*
 * system_tables.c: the level set-up's tables (system_tables.h), in bank $80:
 * the levels' graphics sets at $B7D8 and $B7F5 (a byte by level); the start
 * lists from $ED9D (by level a word, the list's offset from $ED9D; 4 bytes
 * by restart point: x, bit 15 facing left, then y); the camera lists from
 * $EF15 (by level a word, the list's address in the bank; 4 bytes by restart
 * point: x, y).
 */
#include "data/data_internal.h"
#include "data/system_tables.h"

#define LEVEL_BANK 0x80
#define LEVEL_SET_A_ADDR 0xB7D8
#define LEVEL_SET_B_ADDR 0xB7F5
#define LEVEL_STARTS_ADDR 0xED9D
#define LEVEL_CAMERAS_ADDR 0xEF15
#define FACING_LEFT 0x8000u

static uint16_t bank_word(uint16_t addr)
{
    return cart16(cart_offset(LEVEL_BANK, addr));
}

int8_t level_gfx_set_a(uint8_t level)
{
    return (int8_t)cart8(cart_offset(LEVEL_BANK, (uint16_t)(LEVEL_SET_A_ADDR + level)));
}

int8_t level_gfx_set_b(uint8_t level)
{
    return (int8_t)cart8(cart_offset(LEVEL_BANK, (uint16_t)(LEVEL_SET_B_ADDR + level)));
}

LevelStart level_start(uint8_t level, uint8_t restart)
{
    uint16_t at = (uint16_t)(bank_word((uint16_t)(LEVEL_STARTS_ADDR + 2 * level)) + 4 * restart);
    uint16_t camera = (uint16_t)(bank_word((uint16_t)(LEVEL_CAMERAS_ADDR + 2 * level)) + 4 * restart);
    uint16_t x = bank_word((uint16_t)(LEVEL_STARTS_ADDR + at));
    LevelStart s;

    s.x = (uint16_t)(x & ~FACING_LEFT);
    s.facing_left = (x & FACING_LEFT) != 0;
    s.y = bank_word((uint16_t)(LEVEL_STARTS_ADDR + 2 + at));
    s.camera_x = bank_word(camera);
    s.camera_y = bank_word((uint16_t)(camera + 2));
    return s;
}
