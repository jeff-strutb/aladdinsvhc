/*
 * level_tables.c: values the level takes from per-level tables.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Per level: two bytes for LEVEL_035C (bank $81, word per level). */
#define LEVEL_035C_TABLE 0xB933u
/* Per level, the offset of its trigger-spot list from LEVEL_SPOT_TABLE;
   the list is a count, then 6-byte spots. */
#define LEVEL_SPOT_TABLE 0xEC1Eu

/*
 * @routine 81:878B Level035CSet
 * WHAT IT DOES: Sets $035C-$035D from the level's entry in the table at
 * $81:B933.
 * HOW: The level * 2 indexes the table; its two bytes are copied.
 * Entry: M 8-bit, X 8-bit; DB = $81. Exit: A low byte = the second byte;
 * N, Z from it; X = the level * 2; C = the level's bit 7; Y, V unchanged.
 */
void abi_Level035CSet(void)
{
    uint8_t lv = rd8(abs_db(LEVEL)), v;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 7));
    cpu.x = (uint8_t)(lv << 1);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + cpu.x)));
    wr8(abs_db(LEVEL_035C), v);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + 1 + cpu.x)));
    wr8(abs_db(LEVEL_035C + 1), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:8716 LevelSpotsSet
 * WHAT IT DOES: Finds the level's trigger spots: how many, and where their
 * list is.
 * HOW: The level's word in the spot table is the list's offset; its first
 * byte is the count.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M and X 8-bit;
 * A low byte = the count, high byte = the level word * 2's; X = the low
 * byte of the list's offset; Y = the level * 2 (low byte); N, Z from the
 * count; C = bit 15 of the level word; V unchanged.
 */
void abi_LevelSpotsSet(void)
{
    uint16_t lv, x;
    uint8_t n;

    op_rep(P_M | P_X);
    lv = rd16(abs_db(LEVEL));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 15));
    cpu.a = (uint16_t)(lv << 1);
    cpu.y = cpu.a;
    x = rd16(abs_db((uint16_t)(LEVEL_SPOT_TABLE + cpu.y)));
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    op_sep(P_M);
    n = rd8(abs_db((uint16_t)(LEVEL_SPOT_TABLE + x)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
    cpu.p = nz8(cpu.p, n);
    wr8(abs_db(LEVEL_SPOT_COUNT), n);
    wr16(abs_db(LEVEL_SPOTS), x);
    op_sep(P_X);
}
