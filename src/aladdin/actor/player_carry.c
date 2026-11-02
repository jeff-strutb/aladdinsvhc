/*
 * player_carry.c: actors and the player beside them: carried along when
 * riding (as the actor steps), and faced. Drafted by cdraft.py; named
 * and described by hand.
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
#define TARGET_X 0x1Au              /* where some actors move to: x pixels (word) */
#define TARGET_Y 0x1Eu              /* and y pixels (word) */

void abi_ActorMoveByVector(void);
void abi_ActorMoveByVectorX(void);
void abi_ActorMoveByVectorY(void);
void abi_PlayerSpotAboveActor(void);
void abi_PlayerToActorAbove(void);

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

/*
 * @routine 83:C1EC ActorStepCarry
 * WHAT IT DOES: Moves the actor a step along its facing (vector set $0E)
 * and carries the player riding it along.
 * HOW: ActorMoveByVector with X = $0E; then ActorCarryPlayer.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB the vectors' bank;
 * called by JSR. Exit: as ActorCarryPlayer.
 */
void abi_ActorStepCarry(void)
{
    ldx8_v(0x0E);  /* 83C1EC ldx */
    abi_ActorMoveByVector();  /* 83C1EE jsl */
    abi_ActorCarryPlayer();  /* 83C1F2 jsr */
    return;  /* 83C1F5 rts */
}


/*
 * @routine 83:C1F6 ActorStepToTargetCarry
 * WHAT IT DOES: Moves the actor a step toward its target ($1A, $1E):
 * across and up or down by vector set 8 on each axis it has not reached;
 * then carries the player riding it along.
 * HOW: Its x pixels (16-bit) unequal to TARGET_X: ActorMoveByVectorX with
 * X = 8; its y pixels unequal to TARGET_Y: ActorMoveByVectorY with X = 8;
 * then ActorCarryPlayer.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB the vectors' bank;
 * called by JSR. Exit: as ActorCarryPlayer.
 */
void abi_ActorStepToTargetCarry(void)
{
    op_rep(0x20);  /* 83C1F6 rep */
    lda16(ea_dp(0x0C));  /* 83C1F8 lda */
    cmp16_v(rd16(ea_dp(0x1A)));  /* 83C1FA cmp */
    if (IF_Z) goto L_83C208;  /* 83C1FC beq */
    op_sep(0x20);  /* 83C1FE sep */
    ldx8_v(0x08);  /* 83C200 ldx */
    abi_ActorMoveByVectorX();  /* 83C202 jsl */
    op_rep(0x20);  /* 83C206 rep */
L_83C208:
    lda16(ea_dp(0x0F));  /* 83C208 lda */
    cmp16_v(rd16(ea_dp(0x1E)));  /* 83C20A cmp */
    if (IF_Z) goto L_83C216;  /* 83C20C beq */
    op_sep(0x20);  /* 83C20E sep */
    ldx8_v(0x08);  /* 83C210 ldx */
    abi_ActorMoveByVectorY();  /* 83C212 jsl */
L_83C216:
    op_sep(0x20);  /* 83C216 sep */
    abi_ActorCarryPlayer();  /* 83C218 jsr */
    return;  /* 83C21B rts */
}


/*
 * @routine 83:FBD6 ActorCarryPlayerLong
 * WHAT IT DOES: ActorCarryPlayer for callers in other banks.
 * HOW: JSR ActorCarryPlayer, RTL.
 * Entry: as ActorCarryPlayer; called by JSL. Exit: as ActorCarryPlayer.
 */
void abi_ActorCarryPlayerLong(void)
{
    abi_ActorCarryPlayer();  /* 83FBD6 jsr */
    return;  /* 83FBD9 rtl */
}

/*
 * @routine 83:FB57 ActorLiftPlayerStep
 * WHAT IT DOES: A step of an actor that lifts the player (from 83:C245):
 * at $44 = 0, when the test at 82:88CF finds the player, the player is
 * placed on it and $44 counts on; negative $44 counts on; positive, the
 * player is kept on it while $1427 is set, else $44 becomes $F0.
 * HOW: $44 zero: the test at 82:88CF; C clear: the player's $03-$04 =
 * $D99B, PlayerSpotAboveActor with X, $44 up. Negative: $44 up. Positive:
 * $1427 set, PlayerToActorAbove with X; else $44 = $F0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; as the call made.
 */
void abi_ActorLiftPlayerStep(void)
{
    lda8(ea_dp(0x44));  /* 83FB57 lda */
    if (IF_Z) goto L_83FB67;  /* 83FB59 beq */
    if (IF_N) goto L_83FB7C;  /* 83FB5B bmi */
    lda8(ea_abs(0x1427));  /* 83FB5D lda */
    if (!IF_Z) goto L_83FB7F;  /* 83FB60 bne */
    lda8_v(0xF0);  /* 83FB62 lda */
    sta8(ea_dp(0x44));  /* 83FB64 sta */
L_83FB66:
    return;  /* 83FB66 rts */
L_83FB67:
    call_routine(0x8288CFu, 1);  /* 83FB67 jsl */
    if (IF_C) goto L_83FB66;  /* 83FB6B bcs */
    op_rep(0x20);  /* 83FB6D rep */
    lda16_v(0xD99B);  /* 83FB6F lda */
    sta16(ea_abs(0x08DE));  /* 83FB72 sta */
    op_sep(0x20);  /* 83FB75 sep */
    txa8();  /* 83FB77 txa */
    abi_PlayerSpotAboveActor();  /* 83FB78 jsl */
L_83FB7C:
    MEM8(inc, ea_dp(0x44));  /* 83FB7C inc */
    return;  /* 83FB7E rts */
L_83FB7F:
    txa8();  /* 83FB7F txa */
    abi_PlayerToActorAbove();  /* 83FB80 jsl */
    return;  /* 83FB84 rts */
}

