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

void abi_ActorMoveByVector(void);
void abi_ActorMoveByVectorX(void);
void abi_ActorMoveByVectorY(void);
void abi_PlayerSpotAboveActor(void);
void abi_PlayerToActorAbove(void);

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


