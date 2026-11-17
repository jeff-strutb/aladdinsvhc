/*
 * kind3d.c: actor kind $3D, a thing the player can stand on and ride:
 * its contact with him each frame it waits, a child it starts and its bob. Written from the bytes where
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
void abi_ActorBobReturnStep(void);
void abi_ActorBobStep(void);
void abi_ActorKeepSpot(void);
void abi_AnimStart(void);
void abi_ChildStartListed(void);
void abi_SoundQueueAdd(void);
void abi_ActorEndFarWithChildren(void);
void abi_ActorEndPlaced(void);
void abi_ActorStartChild1E(void);
void abi_ActorStartTwoChildren(void);
void abi_Kind3DBob(void);
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
#define K3D_RECORDS_NEEDED 0xD296u /* in bank $80: by $07, the free list's top it needs at least */
#define K3D_CHILD_SETS 0xC324u    /* in bank $83: by $07, the code that starts its children (words) */

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

/*
 * @routine 83:C332 ActorStartChild1E5
 * WHAT IT DOES: Starts the actor's one child (set 1 of its table at
 * 83:C324): at spawn point $1E, its $07 = 5.
 * HOW: ChildStartListed with X = $1E, Y = 5. No script runs it: written
 * from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor's table at 83:C324 (83:C317). Exit: as ChildStartListed.
 */
void abi_ActorStartChild1E5(void)
{
    ldx8_v(0x1E);
    ldy8_v(0x05);
    abi_ChildStartListed();
}

/* `n` frames (BRK at brk_at first) of ActorKeepSpot and `step`. */
static void bob_frames(uint8_t n, uint32_t brk_at, void (*step)(void))
{
    lda8_v(n);
    sta8(ea_dp(ACTOR_40));
    do {
        actor_yield(brk_at);
        abi_ActorKeepSpot();
        step();
        MEM8(dec, ea_dp(ACTOR_40));
    } while (!IF_Z);
}

/*
 * @routine 83:C363 Kind3DBob
 * @actor
 * WHAT IT DOES: Actor kind $3D with $07 4 and up: a thing the player can
 * ride (Kind3DWait) that bobs: 40 frames still, 14 frames up (its $4B set
 * at the top, with sound $4E), 27 frames still, 56 frames back down,
 * again, keeping its place each frame for the ride.
 * HOW: $07 = its bit 0; ACTOR_08 = 2; K3D_X_WAS, $42, K3D_Y_WAS, $44 and
 * K3D_RIDDEN 0; AnimStart with $07; ACTOR_09 = $90. Again and again: K3D_4B
 * 0; BRK; ActorKeepSpot; COP $28; 14 frames (BRK first) of ActorKeepSpot
 * and ActorBobStep; K3D_4B up; SoundQueueAdd $4E; BRK; ActorKeepSpot;
 * COP $1B; K3D_4B 0; 56 frames (BRK first) of ActorKeepSpot and
 * ActorBobReturnStep.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * a branch from kind $3D's start ($83:C306). Exit: never returns.
 */
void abi_Kind3DBob(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    stz8(ea_dp(K3D_X_WAS));
    stz8(ea_dp(K3D_X_WAS + 1));
    stz8(ea_dp(K3D_Y_WAS));
    stz8(ea_dp(K3D_Y_WAS + 1));
    stz8(ea_dp(K3D_RIDDEN));
    lda8(ea_dp(ACTOR_07));
    abi_AnimStart();
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    for (;;) {
        stz8(ea_dp(K3D_4B));
        actor_yield(0x83C383u);
        abi_ActorKeepSpot();
        lda8_v(0x28);
        actor_wait(0x83C38Bu);
        bob_frames(0x0E, 0x83C391u, abi_ActorBobStep);
        MEM8(inc, ea_dp(K3D_4B));
        lda8_v(0x4E);
        abi_SoundQueueAdd();
        actor_yield(0x83C3A6u);
        abi_ActorKeepSpot();
        lda8_v(0x1B);
        actor_wait(0x83C3AEu);
        stz8(ea_dp(K3D_4B));
        bob_frames(0x38, 0x83C3B6u, abi_ActorBobReturnStep);
    }
}

/*
 * @routine 83:C302 Kind3DStart
 * @actor
 * WHAT IT DOES: Actor kind $3D: with $07 4 and up the bobbing thing the
 * player rides (Kind3DBob); else, when the free list holds records enough
 * for the set, it starts its children by set $07 (0: ActorStartChild1E,
 * 1: ActorStartChild1E5, 2: ActorStartTwoChildren) and stays, ended with
 * them once it is far from the range point; with too few records it ends.
 * HOW: $07 at least 4: on at Kind3DBob. Else $44 0; ACTOR_FREE_TOP
 * negative or below K3D_RECORDS_NEEDED by $07: JMP ActorEndPlaced (no
 * script runs this). JSR through K3D_CHILD_SETS by $07 times 2; then each
 * frame (BRK first) ActorEndFarWithChildren.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the table
 * K3D_RECORDS_NEEDED's bank); reached by the actor loop's jump for kind
 * $3D in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind3DStart(void)
{
    lda8(ea_dp(ACTOR_07));
    cmp8_v(0x04);
    if (IF_C) {
        abi_Kind3DBob();
        return;
    }
    stz8(ea_dp(ACTOR_44));
    tay8();
    ACC8(asl);
    tax8();
    lda8(ea_abs(ACTOR_FREE_TOP));
    if (IF_N) {
        abi_ActorEndPlaced();
        return;
    }
    cmp8_v(rd8(ea_absy(K3D_RECORDS_NEEDED)));
    if (!IF_C) {
        abi_ActorEndPlaced();
        return;
    }
    switch (cpu.x) {
    case 0: abi_ActorStartChild1E(); break;
    case 2: abi_ActorStartChild1E5(); break;
    case 4: abi_ActorStartTwoChildren(); break;
    default: call_routine(0x830000u | rd16(0x830000u | (uint16_t)(K3D_CHILD_SETS + cpu.x)), 0); break;
    }
    for (;;) {
        actor_yield(0x83C31Au);
        push16(0xC31E);
        abi_ActorEndFarWithChildren();
        cpu.s = (uint16_t)(cpu.s + 2);
    }
}
