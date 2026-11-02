/*
 * anim_sets.c: which animation sets a level's actors need.
 *
 * Each actor type uses one animation set. Loading a level marks the sets
 * of the types it has (the always-present types and the level's own) and
 * lists each set once, so each is loaded once.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_AnimSetsLoad(void);
#include "wram.h"
#include "wram_video.h"
#include "actor_anim.h"

void abi_AnimSetsListEnd(void);
void abi_AnimSetsPlaces(void);

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

    op_push8(cpu.db);                         /* PHB, then the data bank = $82 */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x82);
    op_push8(0x82);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
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
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
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

    op_push8(cpu.db);                         /* PHB, then the data bank = $7F */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    op_push8(0x7F);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
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
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
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

/*
 * @routine 81:9086 AnimSetsPlaces
 * WHAT IT DOES: For each actor type, finds where its animation set is in
 * the level's list of sets.
 * HOW: With the data bank at $7F: AnimSetsListEnd gives the sets listed;
 * for each of the 256 types the place is 4 (not listed) unless its set is
 * found in the list, when it is the set's place (offset / 2); the count
 * of sets left to look at is kept in scratch $0E.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: Y = 0; X as the last
 * type's search left it; $0C = the sets listed; DB as at entry, N and Z
 * from it (PLB).
 */
void abi_AnimSetsPlaces(void)
{
    uint8_t db = cpu.db, left, x = 0, y = 0, a;

    cpu.db = 0x7F;
    a8_load(0x7F);
    abi_AnimSetsListEnd();
    a = (uint8_t)(((uint8_t)cpu.x >> 1) - 1);
    wr8(dba(SCRATCH), a);
    do {
        /* type Y: its set's place in the list, 4 when not there */
        left = rd8(dba(SCRATCH));
        wr8(dba(SCRATCH + 2), left);
        wr8(dba((uint16_t)(ANIM_SET_PLACE_OF_TYPE + y)), 0x04);
        x = 0;
        for (;;) {
            a = rd8(dba((uint16_t)(ANIM_SET_OF_TYPE + y)));
            cmp8_of(a, rd8(dba((uint16_t)(ANIM_SETS_USED + x))));
            if (a == rd8(dba((uint16_t)(ANIM_SETS_USED + x)))) {
                flag_set(P_C, x & 1);
                a = (uint8_t)(x >> 1);
                wr8(dba((uint16_t)(ANIM_SET_PLACE_OF_TYPE + y)), a);
                break;
            }
            x = (uint8_t)(x + 2);
            wr8(dba(SCRATCH + 2), --left);
            if (!left)
                break;
        }
    } while (++y);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = x;
    cpu.y = 0;
    cpu.db = db;
    nz8_of(db);
}

/*
 * @routine 81:8F47 AnimSetsLoad
 * WHAT IT DOES: Loads the level's animation sets: marks and lists the sets
 * its actor types use, and copies each set's picture list and animation
 * tables into work RAM.
 * HOW: With DB = $A6: AnimSetsClear, AnimSetsOfLevel, AnimSetsAlways,
 * AnimSetsList, AnimSetsListEnd. For each set listed: its picture list's
 * place ($7E:2000 + X) goes to anim_set_gfx; its entry in the table at
 * $A6:8000 (set * 3: offset from $8000, bank from $A6) gives a byte count
 * and the bytes, copied through the long pointer at $18 (each step past
 * $FFFF going on at $8000 of the next bank). Then with DB = $A5: for each
 * set, its table's place ($7E:B000 + X) goes to anim_set_table, and the
 * words of its table (offset at $A5:C000 + set * 2; a count / 2 + 1) are
 * copied. Last, AnimSetsPlaces. Scratch $0C counts the sets, $10 indexes
 * the lists, $16 counts bytes or words. The step at $81:8FAD (a count at
 * a bank's last two bytes) is in no set and no script runs it.
 * Entry: DB, P kept on the stack; D = 0; called by JSL. Exit: P and DB as
 * at entry; A, X, Y as AnimSetsPlaces leaves them.
 */
void abi_AnimSetsLoad(void)
{
    uint8_t db = cpu.db, p;
    uint16_t sets, i, x, y, n, set, from;

    op_sep(0x30);
    p = cpu.p;
    cpu.db = 0xA6;
    a8_load(0xA6);
    abi_AnimSetsClear();
    abi_AnimSetsOfLevel();
    abi_AnimSetsAlways();
    abi_AnimSetsList();
    abi_AnimSetsListEnd();
    op_rep(0x30);
    /* each set's picture list: from its entry at $A6:8000 (set * 3: the
       offset from $8000, the bank from $A6), a byte count then the bytes,
       going on at $8000 of the next bank past $FFFF */
    sets = (uint16_t)(((cpu.x & 0xFF) >> 1) - 1);
    dp16_set(0x0C, sets);
    i = 0;
    x = 0;
    do {
        wr16(ANIM_SET_GFX + i, x);
        set = rd16(ANIM_SETS_USED + i);
        i += 2;
        dp16_set(0x10, i);
        y = (uint16_t)(set * 3);
        from = rd16(0xA68000u + y);
        dp16_set(0x18, 0);
        dp16_set(0x1A, (uint16_t)((rd16(0xA68000u + y + 2) & 0x00FF) + 0xA6));
        y = (uint16_t)(from + 0x8000);
        n = rd16(((uint32_t)dp16(0x1A) << 16) + y);
        y += 2;
        if (y < 0x8000) {
            y += 0x8000;
            dp16_set(0x1A, (uint16_t)(dp16(0x1A) + 1));
        }
        do {
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(((uint32_t)dp16(0x1A) << 16) + y));
            wr8(PICTURE_LIST + x, (uint8_t)cpu.a);
            x++;
            y++;
            if (y < 0x8000) {
                y += 0x8000;
                dp16_set(0x1A, (uint16_t)(dp16(0x1A) + 1));
            }
            dp16_set(0x16, --n);
        } while (n);
        dp16_set(0x0C, --sets);
    } while (sets);
    /* each set's animation table: from its offset at $A5:C000 (set * 2),
       a byte size then the words */
    op_sep(0x30);
    cpu.db = 0xA5;
    cpu.y = y & 0xFF;
    abi_AnimSetsListEnd();
    op_rep(0x30);
    sets = (uint16_t)(((cpu.x & 0xFF) >> 1) - 1);
    dp16_set(0x0C, sets);
    i = 0;
    x = 0;
    do {
        wr16(ANIM_SET_TABLE + i, x);
        y = (uint16_t)(rd16(ANIM_SETS_USED + i) << 1);
        i += 2;
        dp16_set(0x10, i);
        y = rd16(0xA5C000u + y);
        n = (uint16_t)((rd16(0xA5C000u + y) >> 1) + 1);
        y += 2;
        do {
            cpu.a = rd16(0xA5C000u + y);
            wr16(ANIM_DATA + x, cpu.a);
            y += 2;
            x += 2;
            dp16_set(0x16, --n);
        } while (n);
        dp16_set(0x0C, --sets);
    } while (sets);
    op_sep(0x30);
    abi_AnimSetsPlaces();
    cpu.p = p;
    cpu.db = db;
    nz8_of(db);
}
