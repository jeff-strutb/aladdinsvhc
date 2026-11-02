/*
 * block_platform.c: pieces of the platform of kind $68 (its start at
 * $83:E6BB), which goes back and forth along a leg and is drawn as blocks
 * of the level map, written ($81:9978) where it moves to, a block at a
 * time. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
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
    op_rep(0x20);
    sec();
    lda16(ea_dp(BLOCK_X));
    sbc16_v(0x0080);
    clc();
    adc16_v(0x0060);
    cmp16_v(0x00C0);
    op_sep(0x20);
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
    op_rep(0x30);
    lda16(ea_dp(BLOCK_ALT));
    and16_v(0x0001);
    ACC16(asl);
    tax16();
    lda16(ea_dp(ACTOR_47));
    ACC16(asl);
    ora16_v(rd16(ea_dp(ACTOR_FACING)));
    and16_v(0x00FF);
    ACC16(asl);
    ACC16(asl);
    clc();
    adc16_v(rd16(ea_absx(BLOCK_STEPS)));
    tax16();
    clc();
    lda16(ea_dp(BLOCK_X));
    adc16_v(rd16(ea_absx(BLOCK_STEPS)));
    sta16(ea_dp(BLOCK_X));
    lda16(ea_absx(BLOCK_STEPS + 2));
    ldy16(ea_dp(BACK_LEG));
    if (!IF_Z) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    clc();
    adc16_v(rd16(ea_dp(BLOCK_Y)));
    sta16(ea_dp(BLOCK_Y));
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
    ldx8(ea_dp(ACTOR_07));
    lda8(ea_dp(ACTOR_47));
    ACC8(asl);
    ora8_v(rd8(ea_dp(BACK_LEG)));
    ACC8(asl);
    ora8_v(rd8(ea_dp(ACTOR_FACING)));
    clc();
    adc8_v(rd8(ea_absx(MOVE_LISTS)));
    tay8();
    ldx8(ea_absy(MOVE_PICKS));
    clc();
    lda8(ea_dp(ACTOR_X));
    adc8_v(rd8(ea_absx(MOVE_PICKS)));
    sta8(ea_dp(ACTOR_X));
    lda8(ea_dp(ACTOR_X + 1));
    adc8_v(rd8(ea_absx(MOVE_PICKS + 1)));
    sta8(ea_dp(ACTOR_X + 1));
    lda8(ea_dp(ACTOR_X + 2));
    adc8_v(rd8(ea_absx(MOVE_PICKS + 2)));
    sta8(ea_dp(ACTOR_X + 2));
    clc();
    lda8(ea_dp(ACTOR_Y));
    adc8_v(rd8(ea_absx(MOVE_PICKS + 3)));
    sta8(ea_dp(ACTOR_Y));
    lda8(ea_dp(ACTOR_Y + 1));
    adc8_v(rd8(ea_absx(MOVE_PICKS + 4)));
    sta8(ea_dp(ACTOR_Y + 1));
    lda8(ea_dp(ACTOR_Y + 2));
    adc8_v(rd8(ea_absx(MOVE_PICKS + 5)));
    sta8(ea_dp(ACTOR_Y + 2));
}
