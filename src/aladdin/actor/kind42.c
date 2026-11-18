/*
 * kind42.c: actor kind $42, the pot a kind $20 guard with $07 bit 4
 * carries over its head (Kind20Carrier, kind20_carrier.c): it rides there
 * until the guard's throw, then flies at Aladdin, rolls or bounces along
 * the ground, and breaks; it drops when the guard is struck. Its wait is
 * Kind42Wait in kind_waits_rest.c. Not in the catalog: no level places
 * such a guard.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorDownedByHarmlessShot(void);
void abi_ActorMotionPreset(void);
void abi_ActorFly(void);
void abi_ActorOutOfGround(void);
void abi_ActorSnapToGround(void);
void abi_ActorMoveX(void);
void abi_SoundQueueAdd(void);
void abi_TileAtProbeLong(void);
void abi_ActorEndPlaced(void);
void abi_Kind42Ahead(void);
void abi_Kind42PastPlayer(void);
void abi_Kind42Break(void);

#define PROBES 0x16u            /* its probe list (word): $D2B2 */
#define LOOK_HIGH 0x2Cu         /* its sprite attributes' high byte */
#define CARRIER 0x40u           /* the guard's record (word) */
#define LEFT 0x41u              /* frames left of its breaking */
#define FRAME 0x27u             /* (in the guard's record) the frame its animation shows */
#define SIGNAL 0x4Fu            /* (in the guard's record) 0; 1 the pot hit Aladdin; 2 it passed him */
#define AHEAD 0xD2B6u           /* (data bank $80) x offsets ahead, facing right then left (words) */

/* A JSL at `site`, its return address on the stack while the callee runs. */
#define JSL(site, fn) do { jsl_frame(site); fn(); cpu.s = (uint16_t)(cpu.s + 3); } while (0)

/*
 * @routine 83:C5DE Kind42Start
 * @actor
 * WHAT IT DOES: The pot's start: it rides on its guard until the guard's
 * animation shows frame $15 (the throw), then flies at Aladdin and, by
 * its $07, rolls along the ground (0) or bounces (else) until it meets a
 * wall or falls too fast; then it breaks (Kind42Break). Its guard gone,
 * it breaks at once; its guard struck (frame $0E or $11), it drops.
 * HOW: AnimStart 0; ACTOR_08 bit 1, ACTOR_09 bit 4; look high byte 6;
 * probes $D2B2. Each frame (BRK first): ActorDownedByHarmlessShot; the
 * guard's state 0: Kind42Break; its frame $0E or $11: the drop; $15: on.
 * ACTOR_09 bit 7 set, ActorMotionPreset $23. $07 0: each frame (BRK
 * first) ActorFly, Kind42PastPlayer, until ActorOutOfGround says Z
 * clear; speed x $0180; vertical speed $0800 or more: Kind42Break; else
 * sound $14 when ACTOR_09 bit 6, ActorSnapToGround, then each frame
 * (BRK first) ActorMoveX, Kind42PastPlayer, Kind42Ahead (a block:
 * Kind42Break), ActorSnapToGround (none: vertical speed 0 and the flight
 * again). $07 else: Kind42Bounces (JMP). The drop: ACTOR_09 bit 7
 * cleared, ActorMotionPreset $24; each frame (BRK first) AnimStep,
 * ActorFly until ActorOutOfGround; Kind42Break.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $42 (the table at $83:8CAB), or by
 * Kind20Carrier's JMP once it swapped its record with the pot's.
 * Exit: never returns.
 */
void abi_Kind42Start(void)
{
    lda8_v(0x00);
    JSL(0x83C5E0u, abi_AnimStart);
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x10);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x06);
    sta8(ea_dp(LOOK_HIGH));
    lda8_v(0xB2);
    sta8(ea_dp(PROBES));
    lda8_v(0xD2);
    sta8(ea_dp(PROBES + 1));
    for (;;) {                          /* carried */
        actor_yield(0x83C5F8u);
        call_frame(0xC5FC, abi_ActorDownedByHarmlessShot);
        op_rep(0x10);
        ldx16(ea_dp(CARRIER));
        lda8(ea_absx(ACTOR_STATE));
        if (IF_Z) {                     /* its guard gone */
            jump_routine(0x83C738u);
            return;
        }
        lda8(ea_absx(FRAME));
        op_sep(0x10);
        cmp8_v(0x0E);
        if (IF_Z)
            goto dropped;
        cmp8_v(0x11);
        if (IF_Z)
            goto dropped;
        cmp8_v(0x15);
        if (IF_Z)
            break;
    }
    lda8_v(0x80);                       /* thrown */
    tsb8(ea_dp(ACTOR_09));
    ldy8_v(0x23);
    JSL(0x83C620u, abi_ActorMotionPreset);
    lda8(ea_dp(ACTOR_07));
    if (!IF_Z) {
        jump_routine(0x83C714u);
        return;
    }
    for (;;) {
        do {                            /* in flight */
            actor_yield(0x83C62Bu);
            JSL(0x83C62Du, abi_ActorFly);
            call_frame(0xC633, abi_Kind42PastPlayer);
            JSL(0x83C634u, abi_ActorOutOfGround);
        } while (IF_Z);
        op_rep(0x20);
        lda16_v(0x0180);
        sta16(ea_dp(ACTOR_SPEED_X));
        lda16(ea_dp(ACTOR_SPEED_Y));
        cmp16_v(0x0800);
        op_sep(0x20);
        if (IF_C) {
            jump_routine(0x83C738u);
            return;
        }
        lda8(ea_dp(ACTOR_09));
        and8_v(0x40);
        if (!IF_Z) {
            lda8_v(0x14);
            JSL(0x83C652u, abi_SoundQueueAdd);
        }
        JSL(0x83C656u, abi_ActorSnapToGround);
        actor_yield(0x83C65Au);
        for (;;) {                      /* rolling */
            JSL(0x83C65Cu, abi_ActorMoveX);
            call_frame(0xC662, abi_Kind42PastPlayer);
            call_frame(0xC665, abi_Kind42Ahead);
            if (!IF_Z) {
                jump_routine(0x83C738u);
                return;
            }
            JSL(0x83C668u, abi_ActorSnapToGround);
            if (!IF_C)
                break;
            actor_yield(0x83C66Eu);
        }
        stz8(ea_dp(ACTOR_SPEED_Y));     /* off an edge: in flight again */
        stz8(ea_dp(ACTOR_SPEED_Y + 1));
        stz8(ea_dp(ACTOR_SPEED_Y + 2));
    }
dropped:
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    ldy8_v(0x24);
    JSL(0x83C683u, abi_ActorMotionPreset);
    do {
        actor_yield(0x83C687u);
        JSL(0x83C689u, abi_AnimStep);
        JSL(0x83C68Du, abi_ActorFly);
        JSL(0x83C691u, abi_ActorOutOfGround);
    } while (IF_Z);
    jump_routine(0x83C738u);
}

/*
 * @routine 83:C69A Kind42Ahead
 * WHAT IT DOES: Gives the class of the map's block ahead of the pot (by
 * the table at $80:D2B6 and its facing) at its own height.
 * HOW: Its x pixels plus the table's word by facing into SCRATCH, its y
 * pixels into SCRATCH + 2 (an LDA #$0010 the code loads over), then
 * TileAtProbeLong.
 * Entry: M 8-bit, X 8-bit; D = the pot; DB = $80; called by JSR.
 * Exit: as TileAtProbeLong.
 */
void abi_Kind42Ahead(void)
{
    lda8(ea_dp(ACTOR_FACING));
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(AHEAD));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_abs(SCRATCH));
    lda16_v(0x0010);
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_abs(SCRATCH + 2));
    JSL(0x83C6B1u, abi_TileAtProbeLong);
}

/*
 * @routine 83:C6B6 Kind42PastPlayer
 * WHAT IT DOES: Once the flying pot is past Aladdin (more than 16 pixels
 * beyond him the way it faces), tells its guard (its SIGNAL 2) unless the
 * guard has been told already.
 * HOW: 16-bit registers, C clear. Facing right: Aladdin's x + $10 below
 * the pot's x; facing left: Aladdin's x - $10 (SBC #$0F with C clear) at
 * or above it. Then the guard's SIGNAL 0: 2. M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the pot; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M and X 8-bit.
 */
void abi_Kind42PastPlayer(void)
{
    lda8(ea_dp(ACTOR_FACING));
    op_rep(0x31);
    if (IF_Z) {
        lda16(ea_abs(PLAYER + ACTOR_X + 1));
        adc16_v(0x0010);
        cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
        if (IF_C)
            goto out;
    } else {
        lda16(ea_abs(PLAYER + ACTOR_X + 1));
        sbc16_v(0x000F);
        cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
        if (!IF_C)
            goto out;
    }
    ldx16(ea_dp(CARRIER));
    op_sep(0x20);
    lda8(ea_absx(SIGNAL));
    if (IF_Z) {
        lda8_v(0x02);
        sta8(ea_absx(SIGNAL));
    }
out:
    op_sep(0x30);
}

/*
 * @routine 83:C714 Kind42Bounces
 * @actor
 * WHAT IT DOES: The thrown pot with $07 nonzero bounces along: each frame
 * it flies on; a block ahead breaks it (Kind42Break); landed, it rests 2
 * frames and springs up again.
 * HOW: Each frame (BRK first): ActorFly; Kind42Ahead nonzero: Kind42Break;
 * Kind42PastPlayer; ActorOutOfGround Z: again; else ActorSnapToGround,
 * COP 2, ActorMotionPreset $25, again.
 * Entry: M 8-bit, X 8-bit; D = the pot; DB = $80; reached by Kind42Start's
 * JMP. Exit: never returns.
 */
void abi_Kind42Bounces(void)
{
    for (;;) {
        actor_yield(0x83C714u);
        JSL(0x83C716u, abi_ActorFly);
        call_frame(0xC71C, abi_Kind42Ahead);
        if (!IF_Z) {
            abi_Kind42Break();          /* its branch there */
            return;
        }
        call_frame(0xC721, abi_Kind42PastPlayer);
        JSL(0x83C722u, abi_ActorOutOfGround);
        if (IF_Z)
            continue;
        JSL(0x83C728u, abi_ActorSnapToGround);
        lda8_v(0x02);
        actor_wait(0x83C72Eu);
        ldy8_v(0x25);
        JSL(0x83C732u, abi_ActorMotionPreset);
    }
}

/*
 * @routine 83:C738 Kind42Break
 * @actor
 * WHAT IT DOES: The pot breaks: sound $15 if it could be hit, then its
 * breaking animation for 16 frames, and its end.
 * HOW: 8-bit registers; ACTOR_09 bit 7 cleared; bit 6 set: sound $15.
 * AnimStart 1; LEFT $10; each frame (BRK first) AnimStep until LEFT runs
 * out; ActorEndPlaced (JMP).
 * Entry: D = the pot; DB = $80; reached by the actor loop's jump for kind
 * $42 in a state with bit 7 set (the table at $83:8DFD) and by
 * Kind42Start's jumps. Exit: never returns.
 */
void abi_Kind42Break(void)
{
    op_sep(0x30);
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_09));
    and8_v(0x40);
    if (!IF_Z) {
        lda8_v(0x15);
        JSL(0x83C746u, abi_SoundQueueAdd);
    }
    lda8_v(0x01);
    JSL(0x83C74Cu, abi_AnimStart);
    lda8_v(0x10);
    sta8(ea_dp(LEFT));
    do {
        actor_yield(0x83C754u);
        JSL(0x83C756u, abi_AnimStep);
        MEM8(dec, ea_dp(LEFT));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}
