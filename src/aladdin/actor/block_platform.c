/*
 * block_platform.c: pieces of the platform of kind $68 (its start at
 * $83:E6BB), which goes back and forth along a leg and is drawn as blocks
 * of the level map, written ($81:9978) where it moves to, a block at a
 * time. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

/* The platform's own fields of its record */
#define BLOCK_X 0x42u              /* where its next blocks are written: x, y pixels (words) */
#define BLOCK_Y 0x44u
#define BACK_LEG 0x19u             /* 0 on the way out, 1 on the way back (word) */
#define BLOCK_ALT 0x4Au            /* bit 0 flips at every block step */

/* In the bank DB names: by bit 0 of BLOCK_ALT, the offset from here of a
   list of block steps (words), 4 bytes each: x then y (words), picked by
   ACTOR_47 and the facing. */
#define BLOCK_STEPS 0xD5A3u
/* In the bank DB names: by ACTOR_07, the start of its list in MOVE_PICKS;
   the list's bytes, picked by ACTOR_47, the leg and the facing, are
   offsets from MOVE_PICKS of 6-byte moves: x then y (24-bit, fraction
   first). */
#define MOVE_LISTS 0xD517u
#define MOVE_PICKS 0xD51Fu

/*
 * @routine 83:E7DE BlockPointInBounds
 * WHAT IT DOES: Tells whether the platform's block point is in the part
 * of the map its blocks may be written to: C clear when its x pixel is
 * from $20 to $DF.
 * HOW: x less $80, plus $60, compared unsigned with $C0, with 16-bit A.
 * Entry: M 8-bit; D = the platform's record. Exit: A = the x pixel less
 * $20 (16 bits); N, Z and C from the compare; V from the addition. M
 * 8-bit; X and Y unchanged.
 */
void abi_BlockPointInBounds(void)
{
    uint16_t d;

    op_rep(P_M);
    flag_set(P_C, 1);
    d = op_sbc16(dp16(BLOCK_X), 0x0080, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x0060, &cpu.p);      /* x from $20 to $DF */
    cpu.a = d;
    cmp16_of(d, 0x00C0);
    op_sep(P_M);
}

/*
 * @routine 83:E837 BlockPointStep
 * WHAT IT DOES: Moves the platform's block point on by one block step:
 * the step from the list by bit 0 of BLOCK_ALT, picked by ACTOR_47 and the
 * facing, its y step turned round on the way back.
 * HOW: With 16-bit registers: the list's offset from the table by the
 * bit, plus 4 times (ACTOR_47 twice, or the facing word, low byte); x and
 * y added from the entry, y negated when the back-leg word is nonzero.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
 * bank. Exit: A = the new block y (16 bits); N, V, Z and C from its
 * addition; X = the step's offset (its low byte); Y = the back-leg word's
 * low byte. M 8-bit, X 8-bit.
 */
void abi_BlockPointStep(void)
{
    uint16_t alt, pick, dy;

    op_rep(0x30);
    alt = (uint16_t)((dp16(BLOCK_ALT) & 0x0001) << 1);
    pick = (uint16_t)((((dp16(ACTOR_47) << 1) | dp16(ACTOR_FACING)) & 0x00FF) << 2);
    flag_set(P_C, 0);
    cpu.x = op_adc16(pick, rd16(dba(BLOCK_STEPS) + alt), &cpu.p);
    /* x by the step; y by it, turned on the way back */
    flag_set(P_C, 0);
    cpu.a = op_adc16(dp16(BLOCK_X), rd16(dba(BLOCK_STEPS) + cpu.x), &cpu.p);
    dp16_set(BLOCK_X, cpu.a);
    dy = rd16(dba(BLOCK_STEPS + 2) + cpu.x);
    cpu.y = dp16(BACK_LEG);
    if (cpu.y)
        dy = (uint16_t)((dy ^ 0xFFFF) + 1);
    flag_set(P_C, 0);
    cpu.a = op_adc16(dy, dp16(BLOCK_Y), &cpu.p);
    dp16_set(BLOCK_Y, cpu.a);
    op_sep(0x30);
}

/*
 * @routine 83:E86A BlockPlatformMove
 * WHAT IT DOES: Moves the platform a frame along its leg: x and y by the
 * move its kind's list picks by ACTOR_47, the leg and the facing.
 * HOW: The pick ((ACTOR_47 * 2 | leg) * 2 | facing) indexes the list
 * ACTOR_07 starts; its byte is the move's offset; two 24-bit additions,
 * a byte at a time.
 * Entry: M 8-bit, X 8-bit; D = the platform's record; DB the tables'
 * bank. Exit: A = the y position's top byte; N, V, Z and C from its
 * addition; X = the move's offset; Y = the pick's place in the list. M
 * 8-bit, X 8-bit.
 */
void abi_BlockPlatformMove(void)
{
    uint8_t pick, v = 0;

    /* the move picked by its variant, $47, the leg and its facing */
    x8_load(rd8(dpa(ACTOR_07)));
    pick = (uint8_t)((uint8_t)(((uint8_t)(rd8(dpa(ACTOR_47)) << 1) | rd8(dpa(BACK_LEG))) << 1) | rd8(dpa(ACTOR_FACING)));
    flag_set(P_C, 0);
    cpu.y = op_adc8(pick, rd8(dba(MOVE_LISTS) + cpu.x), &cpu.p);
    x8_load(rd8(dba(MOVE_PICKS) + cpu.y));
    /* x and y (24-bit each) moved by it */
    for (int axis = 0; axis < 2; axis++) {
        uint16_t at = axis ? ACTOR_Y : ACTOR_X;

        flag_set(P_C, 0);
        for (int i = 0; i < 3; i++) {
            v = op_adc8(rd8(dpa((uint16_t)(at + i))), rd8(dba(MOVE_PICKS + 3 * axis + i) + cpu.x), &cpu.p);
            wr8(dpa((uint16_t)(at + i)), v);
        }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}
