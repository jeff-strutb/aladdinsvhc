/*
 * player_scene_moves.c: scripted figures of Aladdin (bank $A5: started at
 * his place, their way picked by $07 through the table at $A5:E56D): the
 * ways that run and leap, and that hop for good. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorSpeedYFromA5(void);
void abi_AnimStart(void);

#define SCENE_CUE 0x146Cu  /* counted up by scripted actors as the scene goes on */
#define HOP_TIME 0x42u  /* frames left of its run */

/* Each frame (BRK at `brk_at` first) until the scene's cue is given. */
static void wait_cue(uint32_t brk_at)
{
    uint8_t cue;

    do {
        actor_yield(brk_at);
        cue = rd8(dba(SCENE_CUE));
        a8_load(cue);
    } while (!cue);
}

/*
 * @routine A5:EC45 PlayerSceneLeap
 * @actor
 * WHAT IT DOES: The scripted figure's way 2: it stands $20 frames, runs
 * along the ground until it leaves it, then leaps; in the air until the
 * scene's cue, then holds a pose until the next cue, and stays.
 * HOW: Animation 0, COP $20; animation 2, ActorSpeedXFromA5 $0F; each
 * frame ActorMoveX and ActorSnapToGround while Z is clear; SCENE_CUE
 * cleared, animation $0C, gravity $38, ActorSpeedYFromA5 $12; each frame
 * ActorFly until SCENE_CUE is set; it is cleared, animation $0D; BRK until
 * SCENE_CUE is set; bit 7 of $09 cleared; COP $7F for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 2. Exit: never returns.
 */
void abi_PlayerSceneLeap(void)
{
    uint8_t cue;

    a8_load(0x00);
    abi_AnimStart();
    a8_load(0x20);
    actor_wait(0xA5EC4Du);
    /* runs till he is off the ground */
    a8_load(0x02);
    abi_AnimStart();
    y8_load(0x0F);
    abi_ActorSpeedXFromA5();
    do {
        actor_yield(0xA5EC5Au);
        abi_ActorMoveX();
        abi_ActorSnapToGround();
    } while (!(cpu.p & P_Z));
    /* leaps, until the cue */
    wr8(dba(SCENE_CUE), 0);
    a8_load(0x0C);
    abi_AnimStart();
    a8_load(0x38);
    wr8(dpa(ACTOR_ACCEL_Y), 0x38);
    y8_load(0x12);
    abi_ActorSpeedYFromA5();
    do {
        actor_yield(0xA5EC78u);
        abi_ActorFly();
        cue = rd8(dba(SCENE_CUE));
        a8_load(cue);
    } while (!cue);
    wr8(dba(SCENE_CUE), 0);
    a8_load(0x0D);
    abi_AnimStart();
    wait_cue(0xA5EC8Cu);
    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);
    for (;;) {
        a8_load(0x7F);
        actor_wait(0xA5EC99u);
    }
}


/* Each frame (BRK at `brk_at` first) falling until he is in the ground. */
static void fall_to_ground(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        abi_ActorFall();
        abi_ActorOutOfGround();
    } while (cpu.p & P_Z);
}

/*
 * @routine A5:EC9D PlayerSceneHops
 * @actor
 * WHAT IT DOES: The scripted figure's way 3: it drops to the ground facing
 * left, then for good: runs $30 frames along the ground, stands, holds a
 * pose, and jumps up and lands again.
 * HOW: $4D = 2, facing and flip 1, gravity $38, ActorSpeedYFromA5 6;
 * ActorFall each frame until ActorOutOfGround. Then for good:
 * ActorSpeedYFromA5 6, ActorSpeedXFromA5 $0C, animation 1, $30 frames of
 * ActorMoveX and ActorSnapToGround; animation 0, COP $30; animation $0A,
 * COP $10; animation 0, gravity $38, ActorSpeedYFromA5 $0C, ActorFall each
 * frame until ActorOutOfGround.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 3. Exit: never returns.
 */
void abi_PlayerSceneHops(void)
{
    a8_load(0x02);
    wr8(dpa(ACTOR_4D), 0x02);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x38);
    wr8(dpa(ACTOR_ACCEL_Y), 0x38);
    y8_load(0x06);
    abi_ActorSpeedYFromA5();
    fall_to_ground(0xA5ECB0u);
    for (;;) {                            /* a hop left, a pause, again */
        y8_load(0x06);
        abi_ActorSpeedYFromA5();
        y8_load(0x0C);
        abi_ActorSpeedXFromA5();
        a8_load(0x01);
        abi_AnimStart();
        a8_load(0x30);
        wr8(dpa(HOP_TIME), 0x30);
        do {
            actor_yield(0xA5ECD0u);
            abi_ActorMoveX();
            abi_ActorSnapToGround();
        } while (dec8_at(dpa(HOP_TIME)));
        a8_load(0x00);
        abi_AnimStart();
        a8_load(0x30);
        actor_wait(0xA5ECE6u);
        a8_load(0x0A);
        abi_AnimStart();
        a8_load(0x10);
        actor_wait(0xA5ECF0u);
        a8_load(0x00);
        abi_AnimStart();
        a8_load(0x38);
        wr8(dpa(ACTOR_ACCEL_Y), 0x38);
        y8_load(0x0C);
        abi_ActorSpeedYFromA5();
        fall_to_ground(0xA5ED01u);
    }
}
