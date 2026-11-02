/*
 * player_reach.c: whether the player is within an actor's reach, by an entry of the reach
 * table. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

/* In bank $80: 8 bytes an entry: the x reach, the y reach (bytes), ... */
#define REACH_TABLE 0xD9B0u
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)

/*
 * @routine 83:FCF5 PlayerInReach
 * WHAT IT DOES: Tells whether the player is within entry X's reach of the
 * actor: C clear when in reach (a reach of 0 is not tested).
 * HOW: The x reach r, nonzero: the actor's x pixels plus r less the
 * player's compared unsigned with 2r; C set (out) returns. Then the y
 * reach r, nonzero: its y pixels plus r less the player's compared with r.
 * 16-bit A; $002C, $002E hold r and 2r.
 * Entry: M 8-bit, X 8-bit; X = the entry's offset; D = the actor's record;
 * DB = $80; called by JSR (by 83:FCA5). The game passes only entry 0,
 * whose reaches are 0 (83:AED2 loads X = 8, then X = 0): the comparisons
 * are reached only by unit_player_in_reach.box. Exit: M 16-bit; C clear:
 * in reach; C set: out. A as the last comparison left it.
 */
void abi_PlayerInReach(void)
{
    op_rep(0x20);  /* 83FCF5 rep */
    clc();  /* 83FCF7 clc */
    lda16(ea_absx(0xD9B0));  /* 83FCF8 lda */
    and16_v(0x00FF);  /* 83FCFB and */
    if (IF_Z) goto L_83FD16;  /* 83FCFE beq */
    sta16(ea_abs(0x002C));  /* 83FD00 sta */
    ACC16(asl);  /* 83FD03 asl */
    sta16(ea_abs(0x002E));  /* 83FD04 sta */
    clc();  /* 83FD07 clc */
    lda16(ea_dp(0x0C));  /* 83FD08 lda */
    adc16_v(rd16(ea_abs(0x002C)));  /* 83FD0A adc */
    sec();  /* 83FD0D sec */
    sbc16_v(rd16(ea_abs(0x08E7)));  /* 83FD0E sbc */
    cmp16_v(rd16(ea_abs(0x002E)));  /* 83FD11 cmp */
    if (IF_C) goto L_83FD2B;  /* 83FD14 bcs */
L_83FD16:
    lda16(ea_absx(0xD9B1));  /* 83FD16 lda */
    and16_v(0x00FF);  /* 83FD19 and */
    if (IF_Z) goto L_83FD2B;  /* 83FD1C beq */
    sta16(ea_abs(0x002C));  /* 83FD1E sta */
    clc();  /* 83FD21 clc */
    adc16_v(rd16(ea_dp(0x0F)));  /* 83FD22 adc */
    sec();  /* 83FD24 sec */
    sbc16_v(rd16(ea_abs(0x08EA)));  /* 83FD25 sbc */
    cmp16_v(rd16(ea_abs(0x002C)));  /* 83FD28 cmp */
L_83FD2B:
    return;  /* 83FD2B rts */
}

