/*
 * player_carry.c: actors and the player beside them: carried along when
 * riding (as the actor steps), and faced. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
#define LIFT_STEP 0x44u             /* the lifting actor's step: 0 waiting, negative counting, positive lifting */
#define V_1427 0x001427u            /* nonzero: the player is kept on the lifting actor */
#define LIFT_RESUME 0xD99Bu         /* the player's code resumed while it is lifted */

void abi_ActorMoveByVector(void);
void abi_ActorMoveByVectorX(void);
void abi_ActorMoveByVectorY(void);
void abi_PlayerSpotAboveActor(void);
void abi_PlayerToActorAbove(void);
void abi_ActorGrabTest(void);
void abi_ActorTestByX(void);
void abi_AnimStep(void);
void abi_ActorTestCarries(void);

/* How far the actor moved in one axis since it was kept (its pixels
   less KEPT), added to the player's and to a second word. */
static void carry_axis(uint16_t now, uint16_t kept, uint16_t player, uint16_t other)
{
    uint16_t moved;

    flag_set(P_C, 1);
    moved = op_sbc16(now, kept, &cpu.p);
    push16(moved);
    flag_set(P_C, 0);
    wr16(dba(player), op_adc16(moved, rd16(dba(player)), &cpu.p));
    pla16();
    flag_set(P_C, 0);
    cpu.a = op_adc16(moved, rd16(dba(other)), &cpu.p);
    wr16(dba(other), cpu.a);
}

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
    uint8_t carries = rd8(dpa(CARRIES));

    a8_load(carries);
    if (!carries)
        return;
    op_rep(P_M);
    carry_axis(dp16(ACTOR_X + 1), dp16(KEPT_X), PLAYER_X, V_143B);
    carry_axis(dp16(ACTOR_Y + 1), dp16(KEPT_Y), PLAYER_Y, V_143E);
    op_sep(P_M);
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
    uint16_t x = dp16(ACTOR_X + 1);
    uint8_t facing;

    op_rep(P_X);
    cpu.x = x;
    facing = x >= rd16(dba(PLAYER_X));    /* 1: he is to its left */
    a8_load(facing);
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_FACING), facing);
    op_sep(P_X);
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
    x8_load(0x0E);
    abi_ActorMoveByVector();
    abi_ActorCarryPlayer();
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
    uint16_t now;

    /* a step toward its target ($1A, $1E) in each axis it is not at */
    op_rep(P_M);
    now = dp16(ACTOR_X + 1);
    cpu.a = now;
    cmp16_of(now, dp16(0x1A));
    if (now != dp16(0x1A)) {
        op_sep(P_M);
        x8_load(0x08);
        abi_ActorMoveByVectorX();
        op_rep(P_M);
    }
    now = dp16(ACTOR_Y + 1);
    cpu.a = now;
    cmp16_of(now, dp16(0x1E));
    if (now != dp16(0x1E)) {
        op_sep(P_M);
        x8_load(0x08);
        abi_ActorMoveByVectorY();
    }
    op_sep(P_M);
    abi_ActorCarryPlayer();
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
 * at $44 = 0, when ActorGrabTest finds the player, the player is
 * placed on it and $44 counts on; negative $44 counts on; positive, the
 * player is kept on it while $1427 is set, else $44 becomes $F0.
 * HOW: $44 zero: ActorGrabTest; C clear: the player's $03-$04 =
 * $D99B, PlayerSpotAboveActor with X, $44 up. Negative: $44 up. Positive:
 * $1427 set, PlayerToActorAbove with X; else $44 = $F0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; as the call made.
 */
void abi_ActorLiftPlayerStep(void)
{
    lda8(ea_dp(LIFT_STEP));  /* 83FB57 lda */
    if (IF_Z) goto L_83FB67;  /* 83FB59 beq */
    if (IF_N) goto L_83FB7C;  /* 83FB5B bmi */
    lda8(ea_abs(V_1427));  /* 83FB5D lda */
    if (!IF_Z) goto L_83FB7F;  /* 83FB60 bne */
    lda8_v(0xF0);  /* 83FB62 lda */
    sta8(ea_dp(LIFT_STEP));  /* 83FB64 sta */
L_83FB66:
    return;  /* 83FB66 rts */
L_83FB67:
    abi_ActorGrabTest();  /* 83FB67 jsl */
    if (IF_C) goto L_83FB66;  /* 83FB6B bcs */
    op_rep(0x20);  /* 83FB6D rep */
    lda16_v(LIFT_RESUME);  /* 83FB6F lda */
    sta16(ea_abs(PLAYER + ACTOR_03));  /* 83FB72 sta */
    op_sep(0x20);  /* 83FB75 sep */
    txa8();  /* 83FB77 txa */
    abi_PlayerSpotAboveActor();  /* 83FB78 jsl */
L_83FB7C:
    MEM8(inc, ea_dp(LIFT_STEP));  /* 83FB7C inc */
    return;  /* 83FB7E rts */
L_83FB7F:
    txa8();  /* 83FB7F txa */
    abi_PlayerToActorAbove();  /* 83FB80 jsl */
    return;  /* 83FB84 rts */
}

/*
 * @routine 83:FB85 ActorTestCarriesLong
 * WHAT IT DOES: ActorTestCarries for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorTestCarries; called by JSL. Exit: as ActorTestCarries.
 */
void abi_ActorTestCarriesLong(void)
{
    abi_ActorTestCarries();
}

/*
 * @routine 83:FB89 ActorTestCarries
 * WHAT IT DOES: Tells, by the test X picks, whether the player rides the
 * actor this frame (CARRIES), keeps its position for carrying him, and
 * plays its animation on.
 * HOW: CARRIES = 0; ActorTestByX with Y = its $4C (the test's box); C
 * clear: CARRIES up to 1. KEPT_X and KEPT_Y = its x and y pixels; then
 * AnimStep.
 * Entry: M 8-bit, X 8-bit; X = the test (0 ActorTestSolid, 2
 * ActorTestLand); D = the actor; DB = $80; called by JSR. Exit: as
 * AnimStep.
 */
void abi_ActorTestCarries(void)
{
    stz8(ea_dp(CARRIES));  /* 83FB89 stz */
    ldy8(ea_dp(ACTOR_4C));  /* 83FB8B ldy */
    abi_ActorTestByX();  /* 83FB8D jsl */
    if (IF_C) goto L_83FB95;  /* 83FB91 bcs */
    MEM8(inc, ea_dp(CARRIES));  /* 83FB93 inc */
L_83FB95:
    op_rep(0x20);  /* 83FB95 rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 83FB97 lda */
    sta16(ea_dp(KEPT_X));  /* 83FB99 sta */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83FB9B lda */
    sta16(ea_dp(KEPT_Y));  /* 83FB9D sta */
    op_sep(0x20);  /* 83FB9F sep */
    abi_AnimStep();  /* 83FBA1 jsl */
}
