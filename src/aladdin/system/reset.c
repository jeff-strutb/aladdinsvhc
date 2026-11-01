/*
 * reset.c: clearing the game's state before a new screen or game.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Work RAM the game clears whole for a new game: $03F9-$1CCD, and the
   1.5 KB at $7E:F400. */
#define GAME_RAM 0x03F9u
#define GAME_RAM_LAST 0x18D4u
#define GAME_RAM_HIGH 0x7EF400u
#define GAME_RAM_HIGH_LAST 0x05FFu
/* The four bytes from $03F5 the level code counts with. */
#define LEVEL_COUNTERS 0x03F5u
/* Two tables of records the level code keeps: 6 of 14 bytes from $15F1, 10
   of 16 bytes from $1645; their first bytes cleared. */
#define TABLE_15F1 0x15F1u
#define TABLE_1645 0x1645u

/*
 * @routine 81:9626 TaskSlotsFree1To6
 * WHAT IT DOES: Ends every task but those in slots 0 and 7.
 * HOW: Clears the state byte of slots 1 to 6.
 * Entry: DB a bank whose $0000-$1FFF is work RAM; M 8-bit. Exit: every
 * register and flag unchanged.
 */
void abi_TaskSlotsFree1To6(void)
{
    int slot;

    for (slot = 1; slot <= 6; slot++)
        wr8(abs_db((uint16_t)(TASK_STATE + 0x18 * slot)), 0);
}

/*
 * @routine 81:9647 LevelCountersClear
 * WHAT IT DOES: Clears the four level counters at $03F5.
 * HOW: Four byte stores, last to first.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: X = $FF; N set, Z clear (DEX); A, Y, C, V unchanged.
 */
void abi_LevelCountersClear(void)
{
    int x;

    op_rep(P_X);
    for (x = 3; x >= 0; x--)
        wr8(abs_db((uint16_t)(LEVEL_COUNTERS + x)), 0);
    cpu.x = 0xFFFF;
    cpu.p = nz16(cpu.p, 0xFFFF);
    op_sep(P_X);
}

/*
 * @routine 81:9655 GameRamClear
 * WHAT IT DOES: Clears the game's state for a new game and ends every task
 * but slot 0's.
 * HOW: Clears $03F9-$1CCD and $7E:F400-$F9FF a byte at a time, last first,
 * then the state byte of slots 7 down to 1.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: X 8-bit = 0; A low byte = 0; N clear, Z set, C set; V clear; Y
 * unchanged.
 */
void abi_GameRamClear(void)
{
    int x;
    uint8_t a;

    op_rep(P_X);
    for (x = GAME_RAM_LAST; x >= 0; x--)
        wr8(abs_db((uint16_t)(GAME_RAM + x)), 0);
    cpu.a &= 0xFF00;
    for (x = GAME_RAM_HIGH_LAST; x >= 0; x--)
        wr8(GAME_RAM_HIGH + x, 0);
    op_sep(P_X);
    x = 0xA8;
    do {
        wr8(abs_db((uint16_t)(TASK_STATE + x)), 0);
        cpu.p |= P_C;
        a = op_sbc8((uint8_t)x, 0x18, &cpu.p);
        x = a;
    } while (x != 0);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.x = 0;
}

/* Clears the first byte of `n` records of `size` bytes from `table`: X
   from 0, stepped by an ADC. */
static void clear_firsts(uint16_t table, uint8_t n, uint8_t size)
{
    uint8_t x = 0;

    cpu.y = n;
    do {
        wr8(abs_db((uint16_t)(table + x)), 0);
        cpu.p &= (uint8_t)~P_C;
        x = op_adc8(x, size, &cpu.p);
        cpu.y = (uint8_t)(cpu.y - 1);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    } while (cpu.y != 0);
    cpu.x = x;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | x);
}

/*
 * @routine 81:968F Table15F1Clear
 * WHAT IT DOES: Empties the level code's six 14-byte records at $15F1.
 * HOW: Clears each record's first byte.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low byte = X = $54; Y = 0; Z set, N clear; C, V from the last
 * ADC (clear).
 */
void abi_Table15F1Clear(void)
{
    clear_firsts(TABLE_15F1, 6, 0x0E);
}

/*
 * @routine 81:967B LevelTablesClear
 * WHAT IT DOES: Empties the level code's record tables at $15F1 and $1645.
 * HOW: Table15F1Clear, then clears the first byte of each of the ten
 * 16-byte records at $1645.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low byte = X = $A0; Y = 0; Z set, N clear; C, V from the last
 * ADC (clear).
 */
void abi_LevelTablesClear(void)
{
    abi_Table15F1Clear();
    clear_firsts(TABLE_1645, 10, 0x10);
}
