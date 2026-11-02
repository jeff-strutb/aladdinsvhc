/*
 * player_box.c: the box actors are tested against, set from the player's
 * box table: its offsets from the player and its half-sizes. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_PlayerBoxSet(void);

/* The test box (see actor/hit_box.c): its centre's offsets from the
   player (x, y), its half-width and half-height (words) */
#define BOX_OFFSET_X 0x00148Cu
#define BOX_OFFSET_Y 0x00148Eu
#define BOX_HALF_W 0x001488u
#define BOX_HALF_H 0x00148Au
#define TEST_BOX_X 0x001484u      /* the test box's centre (words: x, then y) */
#define TEST_BOX_Y 0x001486u
/* In bank $80: 10 bytes a box: x offset, y offset, half-width,
   half-height, and the word the player's $16 gets */
#define PLAYER_BOXES 0xC7EBu

#define BOX_TIMES_2 0x002Cu      /* scratch: the box times 2 */

/*
 * @routine 81:E909 PlayerBoxUpdate
 * WHAT IT DOES: Sets the test box to the player's box wanted, when it is
 * not the one set already.
 * HOW: PLAYER_BOX_WANTED compared with PLAYER_BOX: the same, returns at
 * PlayerBoxSet's RTS; else on into PlayerBoxSet.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR. Exit: the same: A low = the box, Z set; else as PlayerBoxSet.
 */
void abi_PlayerBoxUpdate(void)
{
    uint8_t want = rd8(dba(PLAYER_BOX_WANTED)), have = rd8(dba(PLAYER_BOX));

    a8_load(want);
    cmp8_of(want, have);
    if (want != have)
        abi_PlayerBoxSet();
}


/*
 * @routine 81:E911 PlayerBoxSet
 * WHAT IT DOES: Sets the test box to the player's box A: its offsets from
 * the player, its half-sizes, and the player's $16.
 * HOW: A kept in PLAYER_BOX; times 10 ($002C the double) indexes the box
 * table; the x offset negated when the player's $15 is nonzero; five words
 * copied.
 * Entry: M 8-bit, X 8-bit; A = the box; D = the player's record; DB = $80.
 * Exit: M 8-bit, X 8-bit; X = the box times 10; Y = the player's $15; A =
 * the $16 word.
 */
void abi_PlayerBoxSet(void)
{
    uint8_t box = (uint8_t)cpu.a, twice = (uint8_t)(box << 1);
    uint32_t entry;
    uint16_t offset, word;

    wr8(dba(PLAYER_BOX), box);
    wr8(dba(BOX_TIMES_2), twice);
    flag_set(P_C, 0);
    cpu.x = op_adc8((uint8_t)(twice << 2), rd8(dba(BOX_TIMES_2)), &cpu.p);   /* 10 bytes a box */
    y8_load(rd8(dpa(ACTOR_FLIP)));
    entry = dba(PLAYER_BOXES) + cpu.x;
    offset = rd16(entry);
    if (cpu.y)                            /* facing left: the x offset turned */
        offset = (uint16_t)((offset ^ 0xFFFF) + 1);
    wr16(dba(BOX_OFFSET_X), offset);
    wr16(dba(BOX_OFFSET_Y), rd16(entry + 2));
    wr16(dba(BOX_HALF_W), rd16(entry + 4));
    wr16(dba(BOX_HALF_H), rd16(entry + 6));
    word = rd16(entry + 8);
    dp16_set(0x16, word);
    a16_load(word);
}

/*
 * @routine 81:E8D5 TestBoxPlace
 * WHAT IT DOES: Puts the test box's centre at the actor's place plus the
 * box offsets: where actors are tested against it.
 * HOW: 16-bit adds: its x pixels plus BOX_OFFSET_X to TEST_BOX_X, its y
 * pixels plus BOX_OFFSET_Y to TEST_BOX_Y; then M and X 8-bit.
 * Entry: D = the actor (the player); DB a bank whose $0000-$1FFF is work
 * RAM; reached after a call in the kind 0 wait work ($81:E864) and by its
 * jump at $81:E861. Exit: M and X 8-bit; A = the centre's y; N, V, Z, C
 * from its addition; RTL.
 */
void abi_TestBoxPlace(void)
{
    flag_set(P_C, 0);
    wr16(dba(TEST_BOX_X), op_adc16(dp16(ACTOR_X + 1), rd16(dba(BOX_OFFSET_X)), &cpu.p));
    flag_set(P_C, 0);
    cpu.a = op_adc16(dp16(ACTOR_Y + 1), rd16(dba(BOX_OFFSET_Y)), &cpu.p);
    wr16(dba(TEST_BOX_Y), cpu.a);
    op_sep(0x30);
}
