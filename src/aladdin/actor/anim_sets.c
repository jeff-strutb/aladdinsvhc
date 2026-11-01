/*
 * anim_sets.c: which animation sets a level's actors need.
 *
 * Each actor type uses one animation set. Loading a level marks the sets
 * of the types it has (the always-present types and the level's own) and
 * lists each set once, so each is loaded once.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

#define NO_SET 0xFF
/* Each type's animation set, 256 bytes in bank $81; the first 32 types
   are present in every level. */
#define TYPE_ANIM_SETS 0x81EF6Fu
#define ALWAYS_TYPES 32
/* Per level, the offset (from LEVEL_ACTORS) of its actor list in bank
   $82: a count, then 6-byte entries whose first byte is the type. */
#define LEVEL_ACTORS 0x8A7Au
#define LEVEL_ACTOR_SIZE 6

/*
 * @routine 81:9D4C AnimSetsFill
 * WHAT IT DOES: Sets every actor type's animation set to A.
 * HOW: Stores A in all 256 bytes, the first, then the last down.
 * Entry: M 8-bit, X 8-bit; A low byte = the value. Exit: X = 0; Z set,
 * N clear; A, Y, C, V unchanged.
 */
void abi_AnimSetsFill(void)
{
    uint8_t x = 0;

    do {
        wr8(ANIM_SET_OF_TYPE + x, (uint8_t)cpu.a);
        x--;
    } while (x != 0);
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
}

/*
 * @routine 81:9D4A AnimSetsClear
 * WHAT IT DOES: Marks every actor type as needing no animation set.
 * HOW: AnimSetsFill with $FF.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = $FF; X = 0; Z set, N clear;
 * Y, C, V unchanged.
 */
void abi_AnimSetsClear(void)
{
    set_a8(NO_SET);
    abi_AnimSetsFill();
}

/*
 * @routine 81:906B AnimSetsAlways
 * WHAT IT DOES: Marks the animation sets of the 32 types every level has.
 * HOW: Copies the first 32 bytes of the type table, last first.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: A low byte = type 0's set;
 * X = $FF; N set, Z clear; Y, C, V unchanged.
 */
void abi_AnimSetsAlways(void)
{
    int x;

    for (x = ALWAYS_TYPES - 1; x >= 0; x--) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(TYPE_ANIM_SETS + x));
        wr8(ANIM_SET_OF_TYPE + x, (uint8_t)cpu.a);
    }
    cpu.x = 0xFF;
    cpu.p = nz8(cpu.p, 0xFF);
}

/*
 * @routine 81:9D0C AnimSetsOfLevel
 * WHAT IT DOES: Marks the animation sets of the actor types the level has.
 * HOW: With the data bank at $82: the level's actor list; for each of its
 * entries, the type's set is copied from the type table, counting in
 * scratch $0C.
 * Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; DB as at entry, N and Z
 * from it (PLB); with entries, A = the entry offset past the last (low
 * byte; C, V from its ADC), X = the last type, Y = the low byte of that
 * offset; with none, A = 0, Y = the list's offset, X, C, V unchanged.
 */
void abi_AnimSetsOfLevel(void)
{
    const uint32_t bank = 0x820000u;
    uint16_t y, n;

    op_rep(P_M | P_X);
    y = (uint16_t)(rd16(bank | LEVEL) << 1);
    y = rd16(bank + LEVEL_ACTORS + y);
    cpu.a = (uint16_t)(rd16(bank + LEVEL_ACTORS + y) & 0x00FF);
    if (cpu.a != 0) {
        wr16(bank | SCRATCH, cpu.a);
        y++;
        do {
            uint8_t type = (uint8_t)rd16(bank + LEVEL_ACTORS + y);

            cpu.x = type;
            wr8(ANIM_SET_OF_TYPE + type, rd8(TYPE_ANIM_SETS + type));
            cpu.p &= (uint8_t)~P_C;
            cpu.a = op_adc16(y, LEVEL_ACTOR_SIZE, &cpu.p);
            y = cpu.a;
            n = (uint16_t)(rd16_rmw(bank | SCRATCH) - 1);
            wr16_rmw(bank | SCRATCH, n);
        } while (n != 0);
    }
    cpu.y = y;
    op_sep(P_M | P_X);
    cpu.p = nz8(cpu.p, cpu.db);               /* PLB */
}

/*
 * @routine 81:902C AnimSetsList
 * WHAT IT DOES: Lists each animation set the level's types use, once.
 * HOW: With the data bank at $7F: for each type with a set, looks for the
 * set in the list; if it is not there, it takes the end's place, with a 0
 * after it and a new end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = 0; A low byte and Y
 * as the last type's search leaves them; C from its last compare; DB as at
 * entry, N and Z from it (PLB); V unchanged.
 */
void abi_AnimSetsList(void)
{
    const uint32_t bank = 0x7F0000u;
    uint8_t x = 0, y = 0, a;

    a = NO_SET;
    wr8(ANIM_SETS_USED, a);
    do {
        a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
        cpu.p = cmp8_flags(cpu.p, a, NO_SET);
        if (a != NO_SET) {
            y = 0;
            for (;;) {
                a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
                cpu.p = cmp8_flags(cpu.p, a, rd8(bank | (uint16_t)(ANIM_SETS_USED + y)));
                if (cpu.p & P_Z)
                    break;
                a = rd8(bank | (uint16_t)(ANIM_SETS_USED + y));
                y = (uint8_t)(y + 2);
                cpu.p = cmp8_flags(cpu.p, a, NO_SET);
                if (cpu.p & P_Z) {
                    a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
                    y = (uint8_t)(y - 2);
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    y++;
                    a = 0;
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    y++;
                    a = NO_SET;
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    break;
                }
            }
        }
        x++;
    } while (x != 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = 0;
    cpu.y = y;
    cpu.p = nz8(cpu.p, cpu.db);               /* PLB */
}

/*
 * @routine 81:9079 AnimSetsListEnd
 * WHAT IT DOES: Finds the end of the level's list of animation sets.
 * HOW: Steps through the list a pair at a time until the $FF end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = the end mark's offset
 * + 2 (twice the sets listed, + 2); A low byte = $FF; Z set, C set, N
 * clear; Y, V unchanged.
 */
void abi_AnimSetsListEnd(void)
{
    uint8_t x = 0, a;

    do {
        a = rd8(ANIM_SETS_USED + x);
        x = (uint8_t)(x + 2);
        cpu.p = cmp8_flags(cpu.p, a, NO_SET);
    } while (a != NO_SET);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = x;
}
