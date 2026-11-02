/*
 * player_carry.c: actors and the player beside them: carried along when
 * riding, and faced. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

#define CARRIES 0x43u               /* nonzero: the player rides the actor this frame */
#define KEPT_X 0x48u                /* the actor's x pixels when last kept (word) */
#define KEPT_Y 0x4Au                /* its y pixels when last kept (word) */
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)
#define V_143B 0x00143Bu            /* moved with the player's x (word) */
#define V_143E 0x00143Eu            /* moved with the player's y (word) */

/*
 * @routine 83:FBDA ActorCarryPlayer
 * WHAT IT DOES: Carries the player along with the actor it rides: moves
 * the player (and $143B, $143E) by how far the actor moved since its x and
 * y were kept.
 * HOW: Only when CARRIES is set: its x pixels less KEPT_X added to the
 * player's x pixels and to $143B; its y pixels less KEPT_Y added to the
 * player's y and to $143E; 16-bit A.
 * Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit;
 * not carrying: A low = 0, Z set; else A = the new $143E.
 */
void abi_ActorCarryPlayer(void)
{
    lda8(ea_dp(CARRIES));  /* 83FBDA lda */
    if (IF_Z) goto L_83FC0C;  /* 83FBDC beq */
    op_rep(0x20);  /* 83FBDE rep */
    sec();  /* 83FBE0 sec */
    lda16(ea_dp(0x0C));  /* 83FBE1 lda */
    sbc16_v(rd16(ea_dp(KEPT_X)));  /* 83FBE3 sbc */
    pha16();  /* 83FBE5 pha */
    clc();  /* 83FBE6 clc */
    adc16_v(rd16(ea_abs(PLAYER_X)));  /* 83FBE7 adc */
    sta16(ea_abs(PLAYER_X));  /* 83FBEA sta */
    pla16();  /* 83FBED pla */
    clc();  /* 83FBEE clc */
    adc16_v(rd16(ea_abs(V_143B)));  /* 83FBEF adc */
    sta16(ea_abs(V_143B));  /* 83FBF2 sta */
    sec();  /* 83FBF5 sec */
    lda16(ea_dp(0x0F));  /* 83FBF6 lda */
    sbc16_v(rd16(ea_dp(KEPT_Y)));  /* 83FBF8 sbc */
    pha16();  /* 83FBFA pha */
    clc();  /* 83FBFB clc */
    adc16_v(rd16(ea_abs(PLAYER_Y)));  /* 83FBFC adc */
    sta16(ea_abs(PLAYER_Y));  /* 83FBFF sta */
    pla16();  /* 83FC02 pla */
    clc();  /* 83FC03 clc */
    adc16_v(rd16(ea_abs(V_143E)));  /* 83FC04 adc */
    sta16(ea_abs(V_143E));  /* 83FC07 sta */
    op_sep(0x20);  /* 83FC0A sep */
L_83FC0C:
    return;  /* 83FC0C rts */
}


/*
 * @routine 83:FC96 ActorFacePlayer
 * WHAT IT DOES: Turns the actor to face the player: facing 1 (left) when
 * it is at or right of the player, 0 when left of it.
 * HOW: Its x pixels compared with the player's (16-bit X); the carry
 * rotated into 0 is the facing.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit: M
 * 8-bit, X 8-bit; A low = the facing; X = its x pixels' low byte; C clear.
 */
void abi_ActorFacePlayer(void)
{
    op_rep(0x10);  /* 83FC96 rep */
    ldx16(ea_dp(0x0C));  /* 83FC98 ldx */
    cpx16_v(rd16(ea_abs(PLAYER_X)));  /* 83FC9A cpx */
    lda8_v(0x00);  /* 83FC9D lda */
    ACC8(rol);  /* 83FC9F rol */
    sta8(ea_dp(0x14));  /* 83FCA0 sta */
    op_sep(0x10);  /* 83FCA2 sep */
    return;  /* 83FCA4 rts */
}

