/*
 * spawn_every8.c: a child the player starts every eighth frame. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnFacing(void);

/*
 * @routine 81:DD90 PlayerSpawnEvery8
 * WHAT IT DOES: Every eighth frame (low three bits of $032D 0) starts a
 * child of hit box $34 at the player's spawn point 2, facing as the player
 * faces.
 * HOW: Spawn point 2; the flip kept on the stack and set to the facing;
 * ActorSpawnFacing with hit box $34 and Y = 1; the flip put back. No
 * record free: X 8-bit and the kept flip pulled, not put back.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:D28D). Exit: M 8-bit; started: as
 * ActorSpawnFacing, the flip put back; none free: X 8-bit, the player's
 * flip left as its facing; not this frame: A low nonzero.
 */
void abi_PlayerSpawnEvery8(void)
{
    lda8(ea_abs(0x032D));  /* 81DD90 lda */
    and8_v(0x07);  /* 81DD93 and */
    if (!IF_Z) goto L_81DDAF;  /* 81DD95 bne */
    lda8_v(0x02);  /* 81DD97 lda */
    sta8(ea_dp(0x21));  /* 81DD99 sta */
    lda8(ea_dp(0x15));  /* 81DD9B lda */
    pha8();  /* 81DD9D pha */
    ldx8(ea_dp(0x14));  /* 81DD9E ldx */
    stx8(ea_dp(0x15));  /* 81DDA0 stx */
    lda8_v(0x34);  /* 81DDA2 lda */
    ldy8_v(0x01);  /* 81DDA4 ldy */
    abi_ActorSpawnFacing();  /* 81DDA6 jsl */
    if (IF_N) goto L_81DDB0;  /* 81DDAA bmi */
    pla8();  /* 81DDAC pla */
    sta8(ea_dp(0x15));  /* 81DDAD sta */
L_81DDAF:
    return;  /* 81DDAF rts */
L_81DDB0:
    op_sep(0x10);  /* 81DDB0 sep */
    pla8();  /* 81DDB2 pla */
    return;  /* 81DDB3 rts */
}

