/*
 * rise_steps.c: actor kinds $88 (a thing that flies up a course and back
 * down each time it is sent) and $96 (one that shows itself after its
 * waits). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorYHistoryPush(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define V_1B1A 0x1B1Au  /* nonzero: send the riser */
#define V_1B1B 0x1B1Bu  /* the course to send it on */
#define RISE_UP 0xD7BEu  /* by course, its motion preset up (in the bank DB names) */
#define RISE_DOWN 0xD7C1u  /* by course, its motion preset down */
#define RISE_FLOOR 0x40u  /* word: the y pixels it rests at ($0100) */
#define RISE_COURSE 0x42u  /* the course it was sent on */
#define STEP_WAITS 0xD8FCu  /* by $07, the frames its first wait lasts (in the bank DB names) */
#define STEP_WAIT 0x40u  /* the frames of its second wait */

/*
 * @routine 84:8E6A RiserActor
 * @actor
 * WHAT IT DOES: Actor kind $88: rests at y 256 until it is sent (V_1B1A),
 * then flies up its course (V_1B1B) until it starts down, and comes back
 * down to y 256; its heights are kept in the y history (ActorYHistoryPush)
 * all the while. Sent again as long as V_1B1A stays set.
 * HOW: $09 = $80; resting: y and $40 = $0100, BRK, until V_1B1A; the
 * course to $42, ActorMotionPreset by RISE_UP; each frame ActorFlyAgainst
 * and ActorYHistoryPush while y is at or below $40; then ActorMotionPreset
 * by RISE_DOWN, each frame ActorFly and ActorYHistoryPush until y is back
 * at $40 or lower; then V_1B1A again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $88 in state $0C (through the stub at
 * $83:9319). Exit: never returns.
 */
void abi_RiserActor(void)
{
    uint16_t y;
    uint8_t send;

    a8_load(0x80);
    wr8(dpa(ACTOR_09), 0x80);
    for (;;) {
        /* at rest at y 256 until it is sent */
        dp16_set(ACTOR_Y + 1, 0x0100);
        dp16_set(RISE_FLOOR, 0x0100);
        a16_load(0x0100);
        actor_yield(0x848E79u);
        for (;;) {
            send = rd8(dba(V_1B1A));
            a8_load(send);
            if (!send)
                break;
            /* up on its course's motion until it comes back to its floor */
            x8_load(rd8(dba(V_1B1B)));
            wr8(dpa(RISE_COURSE), (uint8_t)cpu.x);
            y8_load(rd8(dba(RISE_UP) + cpu.x));
            abi_ActorMotionPreset();
            do {
                actor_yield(0x848E8Cu);
                abi_ActorFlyAgainst();
                abi_ActorYHistoryPush();
                y = dp16(ACTOR_Y + 1);
                cpu.a = y;
                cmp16_of(y, dp16(RISE_FLOOR));
            } while (y >= dp16(RISE_FLOOR));
            x8_load(rd8(dpa(RISE_COURSE)));
            y8_load(rd8(dba(RISE_DOWN) + cpu.x));
            abi_ActorMotionPreset();
            do {
                actor_yield(0x848EA8u);
                abi_ActorFly();
                abi_ActorYHistoryPush();
                y = dp16(ACTOR_Y + 1);
                cpu.a = y;
                cmp16_of(y, dp16(RISE_FLOOR));
            } while (y < dp16(RISE_FLOOR));
        }
    }
}


/*
 * @routine 84:9EF5 SteppedShowActor
 * @actor
 * WHAT IT DOES: Actor kind $96. With $07 bit 4 clear: waits the frames
 * STEP_WAITS gives for $07, then as long again, then shows itself (sound
 * $44, animation twice $07) and stays. Bit 4 set: shown at once (animation
 * 1) at its priority bits set, and stays.
 * HOW: Bits 1-3 of $2C cleared. Clear: $09 bit 4 and $2C bits 2-3 set;
 * STEP_WAITS by $07 to $40, COP with it, COP with $40; $08 bits 0-1 and
 * $09 bit 7 set; SoundQueueAdd $44; AnimStart $07 * 2; COP for good. Set:
 * $2C bits 1-3, $08 bit 1, $09 bits $B0 set; AnimStart 1; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $96 in state $0C (through the stub at
 * $83:93AC). Exit: never returns.
 */
void abi_SteppedShowActor(void)
{
    uint8_t variant, wait;

    a8_load(0x0E);
    trb_at(dpa(ACTOR_2C), 0x0E);
    variant = rd8(dpa(ACTOR_07));
    a8_load(variant);
    flag_set(P_Z, !(variant & 0x10));
    if (variant & 0x10) {                 /* shown at once, and stays */
        a8_load(0x0E);
        tsb_at(dpa(ACTOR_2C), 0x0E);
        a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
        wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
        a8_load(0xB0);
        tsb_at(dpa(ACTOR_09), 0xB0);
        a8_load(0x01);
        abi_AnimStart();
        for (;;)
            actor_yield(0x849F46u);
    }
    /* shown after its two waits, with sound $44, and stays */
    a8_load(0x10);
    tsb_at(dpa(ACTOR_09), 0x10);
    a8_load(0x0C);
    tsb_at(dpa(ACTOR_2C), 0x0C);
    x8_load(variant);
    wait = rd8(dba(STEP_WAITS) + cpu.x);
    a8_load(wait);
    wr8(dpa(STEP_WAIT), wait);
    actor_wait(0x849F11u);
    a8_load(rd8(dpa(STEP_WAIT)));
    actor_wait(0x849F15u);
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x03));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    a8_load(0x44);
    abi_SoundQueueAdd();
    variant = rd8(dpa(ACTOR_07));
    a8_load((uint8_t)(variant << 1));
    flag_set(P_C, variant & 0x80);
    abi_AnimStart();
    for (;;)
        actor_wait(0x849F2Eu);
}
