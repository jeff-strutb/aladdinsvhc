/*
 * kind_a2.c: actor kind $A2: a shot another actor sends out, which turns
 * back to it and is taken in, or falls away once its sender is hit.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveRight(void);
void abi_ActorMoveY(void);
void abi_ActorPriorityClear(void);
void abi_ActorSpeedAddVector(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedYClear(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define KINDA2_TIME 0x40u  /* frames left of its way out */
#define KINDA2_SENDER 0x4Cu  /* word: the record of the actor that sent it */
#define SENDER_OUT 0x42u  /* in the sender's record: its shots out (word) */
#define SENDER_HIT 0x47u  /* in the sender's record: nonzero once it is hit */
#define KINDA2_HEADINGS 0xD501u  /* headings by facing (0 right, 3 left) plus $07, in the bank DB names */

/* Its fall away: motion $6D, its priority cleared, until off the screen. */
static void fall_away(void)
{
    uint8_t flags;

    y8_load(0x6D);
    abi_ActorMotionPreset();
    abi_ActorPriorityClear();
    do {
        actor_yield(0x848317u);
        abi_AnimStep();
        abi_ActorFly();
        flags = rd8(dpa(ACTOR_09));
        a8_load(flags);
        flag_set(P_Z, !(flags & 0x40));
    } while (flags & 0x40);
    abi_ActorEndPlaced();
}

/* A frame of its flight along its heading. */
static void fly_heading(void)
{
    x8_load(0x22);
    abi_ActorSpeedAddVector();
    abi_ActorMoveRight();
    abi_ActorMoveY();
}

/* Its sender (record X) within 16 of it across and 24 up or down. */
static int at_sender(void)
{
    uint16_t d;

    flag_set(P_C, 0);
    d = op_adc16(rd16(dba(ACTOR_Y + 1) + cpu.x), 0x0010, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, dp16(ACTOR_Y + 1), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0030);
    if (d >= 0x0030)
        return 0;
    flag_set(P_C, 0);
    d = op_adc16(rd16(dba(ACTOR_X + 1) + cpu.x), 0x0010, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, dp16(ACTOR_X + 1), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0020);
    return d < 0x0020;
}

/*
 * @routine 84:8249 KindA2Sent
 * @actor
 * WHAT IT DOES: Actor kind $A2 started new: a shot sent out by another
 * actor (its record at $4C) along one of its headings for $20 frames; then
 * it turns back and flies on until it reaches its sender, which takes it
 * in (one shot fewer out), or falls away if the sender has been hit
 * meanwhile.
 * HOW: $08 bit 1 and $09 bits $90 set, its look $0600, sound $2F,
 * animation $1A, both speeds cleared; its heading (in $14) from
 * KINDA2_HEADINGS at $07 plus 0 facing right or 3 left. Each frame
 * ActorSpeedAddVector (X = $22), ActorMoveRight and ActorMoveY; after $20
 * frames the heading's bit 3 flipped (back), and on the same each frame:
 * the sender's $47 set: animation $1B, bit 7 of $09 cleared and into the
 * fall of KindA2Hit; else within $30 below and 16 above, and $20 across,
 * of the sender: its $42 down by one, then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A2 in state $0C (through the stub at
 * $83:9416). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA2Sent(void)
{
    uint8_t facing, heading;

    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    dp16_set(ACTOR_LOOK, 0x0600);
    cpu.a = 0x0600;
    a8_load(0x2F);
    abi_SoundQueueAdd();
    a8_load(0x1A);
    abi_AnimStart();
    abi_ActorSpeedXClear();
    abi_ActorSpeedYClear();
    /* out along its heading for 32 frames */
    facing = rd8(dpa(ACTOR_FACING));
    a8_load(facing);
    if (facing)
        a8_load(0x03);
    flag_set(P_C, 0);
    heading = op_adc8((uint8_t)cpu.a, rd8(dpa(ACTOR_07)), &cpu.p);
    cpu.x = heading;
    a8_load(rd8(dba(KINDA2_HEADINGS) + heading));
    wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    a8_load(0x20);
    wr8(dpa(KINDA2_TIME), 0x20);
    do {
        actor_yield(0x848285u);
        fly_heading();
    } while (dec8_at(dpa(KINDA2_TIME)));
    /* then back, till it reaches its sender (one shot fewer out) or the
       sender is hit (it falls away) */
    a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x08));
    wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
    for (;;) {
        op_sep(0x30);
        actor_yield(0x8482A1u);
        fly_heading();
        op_rep(P_X);
        cpu.x = dp16(KINDA2_SENDER);
        nz16_of(cpu.x);
        a8_load(rd8(dba(SENDER_HIT) + cpu.x));
        if ((uint8_t)cpu.a) {
            op_sep(0x30);
            a8_load(0x1B);
            abi_AnimStart();
            a8_load(0x80);
            trb_at(dpa(ACTOR_09), 0x80);
            fall_away();
            return;
        }
        op_rep(0x30);
        if (at_sender()) {
            a16_load((uint16_t)(rd16(dba(SENDER_OUT) + cpu.x) - 1));
            wr16(dba(SENDER_OUT) + cpu.x, cpu.a);
            op_sep(0x30);
            abi_ActorEndPlaced();
            return;
        }
    }
}


/*
 * @routine 84:82F5 KindA2Hit
 * @actor
 * WHAT IT DOES: Actor kind $A2 started in state $4C (hit): its sender has
 * one shot fewer out, and it falls away until it can no longer be hit,
 * then ends.
 * HOW: Sound $12, animation $1B; the sender's ($4C) $42 down by one
 * (8-bit); then ActorMotionPreset $6D and ActorPriorityClear; each frame
 * AnimStep and ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A2 in state $4C (through the stub at
 * $83:941F). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA2Hit(void)
{
    a8_load(0x12);
    abi_SoundQueueAdd();
    a8_load(0x1B);
    abi_AnimStart();
    op_rep(P_X);
    cpu.x = dp16(KINDA2_SENDER);           /* one shot fewer out */
    a8_load((uint8_t)(rd8(dba(SENDER_OUT) + cpu.x) - 1));
    wr8(dba(SENDER_OUT) + cpu.x, (uint8_t)cpu.a);
    op_sep(P_X);
    fall_away();
}
