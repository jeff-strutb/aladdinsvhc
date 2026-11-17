/*
 * kind3d.c: actor kind $3D, a thing the player can stand on and ride:
 * its contact with him each frame it waits. Written from the bytes where
 * no script runs them; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorPlayerBoxCount(void);
void abi_ActorTurnsShotBack(void);
void abi_PlayerEnter8CFacing(void);
void abi_PlayerMoveWithActor(void);

#define K3D_X_WAS 0x41u           /* K3D_X_COUNT as it was the frame before (word) */
#define K3D_Y_WAS 0x43u           /* K3D_Y_COUNT as it was (word) */
#define K3D_X_COUNT 0x45u         /* nonzero: the player within its box in x (word) */
#define K3D_Y_COUNT 0x47u         /* in y (word) */
#define K3D_RIDDEN 0x49u          /* nonzero: the player rides it */
#define K3D_4B 0x4Bu              /* nonzero: touching it from below does not hurt */
#define K3D_X_PIXELS (PLAYER + ACTOR_X + 1)  /* the player's x pixels (word) */
#define K3D_Y_PIXELS (PLAYER + ACTOR_Y + 1)  /* his y pixels (word) */
#define K3D_BELOW 0x003D          /* his y below it, pushed under */
#define K3D_ON 0x003B             /* his y above it, standing on it */
#define K3D_LEFT 0x0010           /* his x left of it, pushed off */
#define K3D_RIGHT 0x0011          /* right of it */

/* $83:C441: pushed off to the side, K3D_Y_COUNT up. */
static void side_push(void)
{
    lda16(ea_dp(ACTOR_X + 1));
    cmp16_v(rd16(ea_abs(K3D_X_PIXELS)));
    if (IF_C)
        sbc16_v(K3D_LEFT);
    else
        adc16_v(K3D_RIGHT);
    sta16(ea_abs(K3D_X_PIXELS));
    MEM16(inc, ea_dp(K3D_Y_COUNT));
}

/*
 * @routine 83:C3E4 Kind3DPlayerContact
 * WHAT IT DOES: Actor kind $3D's contact with the player: once he comes
 * within its box from above he stands on it and rides it (it carries him
 * with its moves); from below he is pushed under it, and hurt when its $07
 * is nonzero and its $4B zero; touching it from the side he is pushed
 * 16 pixels off to its left or 17 to its right.
 * HOW: K3D_X_COUNT and K3D_Y_COUNT (counted by ActorPlayerBoxCount) moved
 * to K3D_X_WAS and K3D_Y_WAS and cleared. Not ridden (K3D_RIDDEN 0):
 * ActorPlayerBoxCount; within on both axes: unless within in y the frame
 * before, its y pixels below his: his y = its y + $3D, PlayerEnter8CFacing
 * as above, then (V_141C nonzero) K3D_Y_COUNT cleared, or on to the side
 * push; not below: his y = its y - $3B, V_1428 and K3D_RIDDEN 1, V_141C 0.
 * The side push: his x = its x - 16 when its x is not below his, else its
 * x + 17; K3D_Y_COUNT up. Ridden: PlayerMoveWithActor,
 * PlayerEnter8CFacing, ActorPlayerBoxCount; V_141C zero and within on both
 * axes: V_1428 up; else K3D_RIDDEN 0. No script runs the push, the landing
 * or the ride: written from their bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: M 8-bit, X 8-bit; as the
 * last step leaves A and the flags.
 */
void abi_Kind3DPlayerContact(void)
{
    op_rep(P_M);
    lda16(ea_dp(K3D_X_COUNT));
    sta16(ea_dp(K3D_X_WAS));
    lda16(ea_dp(K3D_Y_COUNT));
    sta16(ea_dp(K3D_Y_WAS));
    stz16(ea_dp(K3D_X_COUNT));
    stz16(ea_dp(K3D_Y_COUNT));
    ldx8(ea_dp(K3D_RIDDEN));
    if (!IF_Z) {                          /* ridden */
        op_sep(P_M);
        abi_PlayerMoveWithActor();
        abi_PlayerEnter8CFacing();
        op_rep(P_M);
        abi_ActorPlayerBoxCount();
        ldy8(ea_abs(V_141C));
        if (IF_Z) {
            lda16(ea_dp(K3D_X_COUNT));
            if (!IF_Z) {
                lda16(ea_dp(K3D_Y_COUNT));
                if (!IF_Z) {
                    op_sep(P_M);
                    MEM8(inc, ea_abs(V_1428));
                    return;
                }
            }
        }
        op_sep(P_M);
        stz8(ea_dp(K3D_RIDDEN));
        return;
    }
    abi_ActorPlayerBoxCount();
    lda16(ea_dp(K3D_X_COUNT));
    if (IF_Z)
        goto out;
    lda16(ea_dp(K3D_Y_COUNT));
    if (IF_Z)
        goto out;
    ldx8(ea_dp(K3D_Y_WAS));
    if (!IF_Z) {
        side_push();
        goto out;
    }
    op_rep(P_M);
    lda16(ea_dp(ACTOR_Y + 1));
    cmp16_v(rd16(ea_abs(K3D_Y_PIXELS)));
    if (IF_C) {                           /* he is above it: he stands on it */
        sec();
        sbc16_v(K3D_ON);
        sta16(ea_abs(K3D_Y_PIXELS));
        ldx8_v(0x01);
        stx8(ea_abs(V_1428));
        stx8(ea_dp(K3D_RIDDEN));
        ldx8_v(0x00);
        stx8(ea_abs(V_141C));
        goto out;
    }
    adc16_v(K3D_BELOW);                   /* below it: pushed under */
    sta16(ea_abs(K3D_Y_PIXELS));
    op_sep(P_M);
    lda8(ea_dp(ACTOR_07));
    if (!IF_Z) {
        lda8(ea_dp(K3D_4B));
        if (IF_Z)
            abi_PlayerEnter8CFacing();
    }
    op_rep(P_M);
    ldx8(ea_abs(V_141C));
    if (!IF_Z) {
        stz16(ea_dp(K3D_Y_COUNT));
        goto out;
    }
    side_push();
out:
    op_sep(P_M);
}

/*
 * @routine 83:C4A4 Kind3DWait
 * WHAT IT DOES: What actor kind $3D runs each frame it waits: its contact
 * with the player, then a shot that hits it turned back.
 * HOW: Kind3DPlayerContact, ActorTurnsShotBack.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
 * table by kind ($83:8F4F). Exit: as ActorTurnsShotBack.
 */
void abi_Kind3DWait(void)
{
    abi_Kind3DPlayerContact();
    abi_ActorTurnsShotBack();
}
