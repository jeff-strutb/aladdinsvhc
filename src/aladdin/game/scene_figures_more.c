/*
 * scene_figures_more.c: the scripted figure's way 4 from bank $A5's start
 * (A5:E515): dropped in, then a scene played to its cues. (Way 11,
 * A5:F00E GuardPlacedHold, goes on at GuardStop, whose C is held; its C
 * waits with it.)
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"

void abi_ActorAnimRiseFall(void);
void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorPlaceAtSpot(void);
void abi_ActorSpeedXFromA5(void);
void abi_AnimStart(void);

#define SF_SCENE_CUE 0x146Cu    /* counted up by scripted actors as the scene goes on */
#define SF_CUE_BD 0x00BDu       /* the scene's cues */
#define SF_CUE_BE 0x00BEu
#define SF_HURT_COUNT 0x141Bu   /* nonzero while the player is hurt (his state $8C) */
#define SF_PLAYER_07 (PLAYER + ACTOR_07) /* the player's $07 */
#define SF_COUNT 0x42u          /* frames left of a walk */

static void anim(uint8_t a)
{
    a8_load(a);
    abi_AnimStart();
}

static void wait_frames(uint8_t frames, uint32_t cop_at)
{
    a8_load(frames);
    actor_wait(cop_at);
}

static void low_bits_08(uint8_t bits)
{
    a8_load((uint8_t)((rd8(dpa(ACTOR_08)) & 0xFC) | bits));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
}

/* Each frame (BRK at `brk_at`) ActorMoveX, `frames` frames. */
static void walk(uint8_t frames, uint32_t brk_at)
{
    a8_load(frames);
    wr8(dpa(SF_COUNT), frames);
    do {
        actor_yield(brk_at);
        abi_ActorMoveX();
    } while (dec8_at(dpa(SF_COUNT)));
}

/*
 * @routine A5:ED0F PlayerSceneDropIn
 * @actor
 * WHAT IT DOES: The scripted figure's way 4: it drops to the ground and
 * waits, is put at its spot and leaps in, then plays its part of the
 * scene by the cues: posed while the player is hurt, a leap and fall
 * once cue $BD comes, a wait for the player's $07 to be $0D, a short
 * walk, a pose until cue $BE, and walks off for good. No script meets
 * it: written from its bytes.
 * HOW: Each frame (BRK, animation rise or fall) ActorFall until
 * ActorOutOfGround is nonzero. Animation 0, COP $80; ActorPlaceAtSpot
 * with X = $24; facing and flip 0; ACTOR_08's low bits 3; animation $0C,
 * preset $86; each frame (BRK) ActorAnimRiseFall and ActorFly until
 * ActorOutOfGround nonzero; animation 0, COP $30. Then animation $17 each
 * frame (BRK) while SF_HURT_COUNT is 0 (animation $18 each frame while it
 * is not), flip = SF_SCENE_CUE, until SF_CUE_BD is set; facing and flip 0,
 * preset $7B; each frame ActorFly until falling; ACTOR_08's low bits 1;
 * each frame ActorFly until ActorOutOfGround nonzero; animation $19, COP
 * $50; animation 0, BRK until the player's $07 is $0D; animation 1,
 * ActorSpeedXFromA5 0, $40 frames of ActorMoveX; animation 0, BRK until
 * SF_CUE_BE bit 7; animation $1A, COP $80; ActorSpeedXFromA5 $0F,
 * animation 2, $28 frames of ActorMoveX; ActorSpeedXFromA5 $0C,
 * animation 1, ActorMoveX each frame for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 4. Exit: never returns.
 */
void abi_PlayerSceneDropIn(void)
{
    /* dropped to the ground */
    for (;;) {
        abi_ActorFall();
        abi_ActorOutOfGround();
        if (!(cpu.p & P_Z))
            break;
        actor_yield(0xA5ED19u);
        abi_ActorAnimRiseFall();
    }
    anim(0x00);
    wait_frames(0x80, 0xA5ED28u);

    /* put at its spot, and leaps in */
    x8_load(0x24);
    abi_ActorPlaceAtSpot();
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
    low_bits_08(0x03);
    anim(0x0C);
    y8_load(0x86);
    abi_ActorMotionPreset();
    do {
        actor_yield(0xA5ED48u);
        abi_ActorAnimRiseFall();
        abi_ActorFly();
        abi_ActorOutOfGround();
    } while (cpu.p & P_Z);
    anim(0x00);
    wait_frames(0x30, 0xA5ED5Fu);

    /* posed until cue $BD: $18 while the player is hurt, else $17 */
    anim(0x17);
    for (;;) {
        actor_yield(0xA5ED76u);
        a8_load(rd8(dba(SF_HURT_COUNT)));
        if ((uint8_t)cpu.a) {
            anim(0x18);
            do {
                actor_yield(0xA5ED69u);
                a8_load(rd8(dba(SF_HURT_COUNT)));
            } while ((uint8_t)cpu.a);
            anim(0x17);
            continue;
        }
        a8_load(rd8(dba(SF_SCENE_CUE)));
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        a8_load(rd8(dba(SF_CUE_BD)));
        if ((uint8_t)cpu.a)
            break;
    }

    /* a leap and its fall */
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
    y8_load(0x7B);
    abi_ActorMotionPreset();
    do {
        actor_yield(0xA5ED91u);
        abi_ActorAnimRiseFall();
        abi_ActorFly();
        a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
    } while (cpu.a & 0x80);
    low_bits_08(0x01);
    do {
        actor_yield(0xA5EDA6u);
        abi_ActorAnimRiseFall();
        abi_ActorFly();
        abi_ActorOutOfGround();
    } while (cpu.p & P_Z);
    anim(0x19);
    wait_frames(0x50, 0xA5EDBDu);

    /* until the player's $07 is $0D, then a short walk */
    anim(0x00);
    do {
        actor_yield(0xA5EDC5u);
        a8_load(rd8(dba(SF_PLAYER_07)));
        cmp8_of((uint8_t)cpu.a, 0x0D);
    } while ((uint8_t)cpu.a != 0x0D);
    anim(0x01);
    y8_load(0x00);
    abi_ActorSpeedXFromA5();
    walk(0x40, 0xA5EDDDu);

    /* posed until cue $BE, then off */
    anim(0x00);
    do {
        actor_yield(0xA5EDEDu);
        a8_load(rd8(dba(SF_CUE_BE)));
    } while (!(cpu.a & 0x80));
    anim(0x1A);
    wait_frames(0x80, 0xA5EDFCu);
    y8_load(0x0F);
    abi_ActorSpeedXFromA5();
    anim(0x02);
    walk(0x28, 0xA5EE0Du);
    y8_load(0x0C);
    abi_ActorSpeedXFromA5();
    anim(0x01);
    for (;;) {
        actor_yield(0xA5EE22u);
        abi_ActorMoveX();
    }
}
