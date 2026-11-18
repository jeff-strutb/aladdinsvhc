/*
 * kind6d.c: actor kind $6D (the pots and the like that the wait routine
 * Kind6DWait drives): what it does once downed (its state with bit 7
 * set): knocked away, a sound, and off until it is out of range.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define K6D_FRAMES 0x40u  /* frames left before it flies off */
#define K6D_DOWNED 0x48u  /* counted up by Kind6DWait when a shot downs it */
#define K6D_KNOCKED 0x1F  /* the motion preset it is knocked away with */
#define K6D_POSE 0x07     /* its animation once downed */
#define K6D_SOUND 0x0F    /* the sound of its fall */

/*
 * @routine 83:EFF0 Kind6DSecond
 * @actor
 * WHAT IT DOES: Actor kind $6D downed (its state with bit 7 set): knocked
 * away in pose 7, with sound $0F unless a shot downed it while it faced
 * right; it turns to face right, holds 10 frames, then flies each frame
 * until it is out of range.
 * HOW: ActorMotionPreset $1F, AnimStart 7, K6D_FRAMES $0A. K6D_DOWNED zero
 * or its facing with bit 7 set: SoundQueueAdd $0F. Facing bit 7 cleared.
 * Each frame (BRK) AnimStep, K6D_FRAMES times; then each frame (BRK)
 * AnimStep, ActorFly and ActorEndOutOfRange with X = 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $6D in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns; ends when out of range.
 */
void abi_Kind6DSecond(void)
{
    uint8_t facing;

    y8_load(K6D_KNOCKED);
    abi_ActorMotionPreset();
    a8_load(K6D_POSE);
    abi_AnimStart();
    a8_load(0x0A);
    wr8(dpa(K6D_FRAMES), 0x0A);
    a8_load(rd8(dpa(K6D_DOWNED)));
    if ((uint8_t)cpu.a) {
        facing = rd8(dpa(ACTOR_FACING));
        a8_load(facing);
        if (!(facing & 0x80))
            goto face;
    }
    a8_load(K6D_SOUND);
    abi_SoundQueueAdd();
face:
    facing = rd8(dpa(ACTOR_FACING));
    a8_load(facing);
    if (facing & 0x80) {
        a8_load((uint8_t)(facing & 0x7F));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    }
    do {
        actor_yield(0x83F016u);
        abi_AnimStep();
    } while (dec8_at(dpa(K6D_FRAMES)));
    for (;;) {
        actor_yield(0x83F020u);
        abi_AnimStep();
        abi_ActorFly();
        x8_load(0x00);
        /* the JSR's return address on the stack, which ActorEndOutOfRange
           pulls when it ends the actor */
        call_frame(0xF02E, abi_ActorEndOutOfRange);
    }
}
