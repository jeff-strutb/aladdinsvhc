/*
 * kind20_dazed.c: actor kind $20's guard knocked down and dazed, until it
 * gets up or is knocked away. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_Actor44Set40And80(void);
void abi_ActorAccelerateY(void);
void abi_ActorBouncesPlayer(void);
void abi_ActorHitByShotCC(void);
void abi_ActorMotionPreset(void);
void abi_ActorOutOfGround(void);
void abi_ActorPaletteFromRom(void);
void abi_ActorPictureSend(void);
void abi_ActorStepY(void);
void abi_ActorXTo40(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_Kind20Halt(void);
void abi_Kind20Hit(void);

#define DAZE_PLACE 0x07u          /* its place's byte: bit 4, the palette from ROM */
#define DAZE_FRAMES 0x42u         /* frames of the daze left */

/* Knocked off by a bounce: it can no longer be hit (ACTOR_09 bit 7
   cleared by ASL, LSR), state $8C. */
static void bounced(void)
{
    MEM8(asl, ea_dp(ACTOR_09));
    MEM8(lsr, ea_dp(ACTOR_09));
    lda8_v(0x8C);
    sta8(ea_dp(ACTOR_STATE));
}

/*
 * @routine 83:9C49 Kind20Dazed
 * @actor
 * WHAT IT DOES: Actor kind $20's guard knocked down: it can no longer be
 * hit as before, falls to the ground, then sits dazed for 96 frames;
 * coming down on it or a shot from its front knocks it away (Kind20Hit),
 * a shot from behind starts the daze again; then it gets up (Kind20Halt).
 * HOW: ActorXTo40, Actor44Set40And80; facing = flip; ACTOR_09 bit 7
 * cleared; PLACE bit 4: ActorPaletteFromRom with Y = 5. AnimStart 5,
 * ActorPictureSend, ActorMotionPreset with Y = 2. Until ActorOutOfGround
 * says Z clear, each frame (BRK first): ActorBouncesPlayer (bounced:
 * ACTOR_09 bit 7 cleared, state $8C), ActorHitByShotCC, AnimStep,
 * ActorPictureSend, ActorStepY, ActorAccelerateY. Then DAZE_FRAMES = $60;
 * each frame (BRK first): ActorBouncesPlayer bounced: ACTOR_09 bit 7
 * cleared, state $8C, Kind20Hit; ActorHitByShotCC hit (C clear): facing
 * not negative, from the start again, else Kind20Hit; AnimStep,
 * ActorPictureSend. At 0: ACTOR_09 bit 7 set (TSB), Kind20Halt. No script
 * runs the fall, a bounce, a shot or PLACE bit 4: written from their
 * bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $20 through the entry at $83:90E1.
 * Exit: never returns.
 */
void abi_Kind20Dazed(void)
{
again:
    abi_ActorXTo40();
    abi_Actor44Set40And80();
    lda8(ea_dp(ACTOR_FLIP));
    sta8(ea_dp(ACTOR_FACING));
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    lda8(ea_dp(DAZE_PLACE));
    and8_v(0x10);
    if (!IF_Z) {
        ldy8_v(0x05);
        abi_ActorPaletteFromRom();
    }
    lda8_v(0x05);
    abi_AnimStart();
    abi_ActorPictureSend();
    ldy8_v(0x02);
    abi_ActorMotionPreset();
    for (;;) {                            /* falls to the ground */
        abi_ActorOutOfGround();
        if (!IF_Z)
            break;
        actor_yield(0x839C79u);
        abi_ActorBouncesPlayer();
        if (!IF_Z)
            bounced();
        abi_ActorHitByShotCC();
        abi_AnimStep();
        abi_ActorPictureSend();
        abi_ActorStepY();
        abi_ActorAccelerateY();
    }
    lda8_v(0x60);
    sta8(ea_dp(DAZE_FRAMES));
    do {
        actor_yield(0x839CA1u);
        abi_ActorBouncesPlayer();
        if (!IF_Z) {
            bounced();
            abi_Kind20Hit();
            return;
        }
        abi_ActorHitByShotCC();
        if (!IF_C) {
            lda8(ea_dp(ACTOR_FACING));
            if (!IF_N)
                goto again;
            abi_Kind20Hit();
            return;
        }
        abi_AnimStep();
        abi_ActorPictureSend();
        MEM8(dec, ea_dp(DAZE_FRAMES));
    } while (!IF_Z);
    lda8_v(0x80);
    tsb8(ea_dp(ACTOR_09));
    abi_Kind20Halt();
}
