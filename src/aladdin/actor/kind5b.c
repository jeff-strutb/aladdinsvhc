/*
 * kind5b.c: actor kind $5B, a thing that is thrown up and falls away:
 * with $07 bit 7 clear it first shows, waits, leaves a copy of itself
 * (counted in a table by its hit box), then is thrown; with bit 7 set it
 * is thrown at once and lets its wait know it is falling.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStopX(void);
void abi_AnimStart(void);

#define K5B_FRAMES 0x40u        /* frames left of the fall */
#define K5B_07 0x41u            /* its $07, kept over the copy's spawn */
#define K5B_FALLING 0x42u       /* set while the bit-7 kind falls (its wait reads it) */
#define K5B_43 0x43u            /* cleared as it starts */
#define K5B_DELAYS 0xD3F8u      /* by $07: frames before it is thrown (in the bank DB names) */
#define SPAWN_COUNTS 0x7EFF00u  /* by hit box: copies spawned */
#define COPY_HIT_BOX 0x4Au      /* the copy's spawn place */

/* Thrown: no speed across, speed up from entry $3C, gravity 4; each frame
   (BRK at rise_at) flying while it rises; COP $28; then each frame (BRK at
   fall_at) flying for `frames` frames. */
static void thrown(uint32_t rise_at, uint32_t cop_at, uint32_t fall_at, uint8_t frames)
{
    /* its JSLs at fixed places from its first BRK (83:DF63, DF69, DF73)
       and after its second (DF85) */
    jsl_frame(rise_at - 0x0E); abi_ActorStopX(); cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(0x3C);
    jsl_frame(rise_at - 0x08); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x04);
    wr8(dpa(ACTOR_ACCEL_Y), 0x04);
    do {
        actor_yield(rise_at);
        jsl_frame(rise_at + 2); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);
        a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
    } while ((uint8_t)cpu.a & 0x80);
    a8_load(0x28);
    actor_wait(cop_at);
    a8_load(frames);
    wr8(dpa(K5B_FRAMES), frames);
    do {
        actor_yield(fall_at);
        jsl_frame(fall_at + 2); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);
    } while (dec8_at(dpa(K5B_FRAMES)));
}

/*
 * @routine 83:DF14 Kind5BStart
 * @actor
 * WHAT IT DOES: Actor kind $5B. $07 bit 7 clear: pose 2 for its delay
 * (K5B_DELAYS by $07), pose 1 for 32 frames, a copy of itself spawned
 * (counted in SPAWN_COUNTS by its hit box), then thrown up: it rises,
 * waits 40 frames, falls for 128 and ends. Bit 7 set: pose 0, thrown at
 * once with K5B_FALLING set, it falls for 96 frames, K5B_FALLING
 * cleared, and it ends.
 * HOW: K5B_43, K5B_FALLING 0; ACTOR_09 bits $90 set; K5B_07 = $07.
 * Clear: ACTOR_08 bits 0-1 set; AnimStart 2; COP the delay; AnimStart 1;
 * COP $20; ACTOR_SPAWN_AT $4A, $07 $FF, ActorSpawnSameLook with its hit
 * box: spawned, SPAWN_COUNTS by the hit box up; $07 back. Then (both):
 * ActorStopX, ActorSpeedYSet $3C, gravity 4; BRK and ActorFly while the
 * vertical speed is negative; COP $28; frames of BRK and ActorFly; then
 * ActorEndPlaced. Set: ACTOR_08 bit 1 set; AnimStart 0; K5B_FALLING up
 * before, and 0 after.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds K5B_DELAYS (bank $83); reached
 * by the actor loop's jump for kind $5B in state $0C (the table at
 * $83:8CAB). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind5BStart(void)
{
    uint8_t v07;

    wr8(dpa(K5B_43), 0);
    wr8(dpa(K5B_FALLING), 0);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    v07 = rd8(dpa(ACTOR_07));
    a8_load(v07);
    wr8(dpa(K5B_07), v07);
    if (!(v07 & 0x80)) {
        a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x03));
        wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
        a8_load(0x02);
        jsl_frame(0x83DF2Fu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        x8_load(rd8(dpa(ACTOR_07)));
        a8_load(rd8(dba((uint16_t)(K5B_DELAYS + cpu.x))));
        actor_wait(0x83DF38u);
        a8_load(0x01);
        jsl_frame(0x83DF3Cu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        a8_load(0x20);
        actor_wait(0x83DF42u);
        a8_load(COPY_HIT_BOX);
        wr8(dpa(ACTOR_SPAWN_AT), COPY_HIT_BOX);
        a8_load(0xFF);
        wr8(dpa(ACTOR_07), 0xFF);
        a8_load(rd8(dpa(ACTOR_06)));
        jsl_frame(0x83DF4Eu); abi_ActorSpawnSameLook(); cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_N)) {
            uint8_t box = rd8(dpa(ACTOR_06));

            x8_load(box);
            a8_load((uint8_t)(rd8(SPAWN_COUNTS + box) + 1));
            wr8(SPAWN_COUNTS + box, (uint8_t)cpu.a);
        }
        a8_load(rd8(dpa(K5B_07)));
        wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
        thrown(0x83DF71u, 0x83DF7Du, 0x83DF83u, 0x80);
        abi_ActorEndPlaced();
        return;
    }
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load(0x00);
    jsl_frame(0x83DF97u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x83DF9Bu); abi_ActorStopX(); cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(0x3C);
    jsl_frame(0x83DFA1u); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x04);
    wr8(dpa(ACTOR_ACCEL_Y), 0x04);
    (void)inc8_at(dpa(K5B_FALLING));
    do {
        actor_yield(0x83DFABu);
        jsl_frame(0x83DFADu); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);
        a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
    } while ((uint8_t)cpu.a & 0x80);
    a8_load(0x28);
    actor_wait(0x83DFB7u);
    a8_load(0x60);
    wr8(dpa(K5B_FRAMES), 0x60);
    do {
        actor_yield(0x83DFBDu);
        jsl_frame(0x83DFBFu); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);
    } while (dec8_at(dpa(K5B_FRAMES)));
    wr8(dpa(K5B_FALLING), 0);
    abi_ActorEndPlaced();
}
