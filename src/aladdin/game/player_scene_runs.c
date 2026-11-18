/*
 * player_scene_runs.c: the scripted figures of Aladdin (bank $A5)
 * whose ways end by starting over as way 0 at GuardRestart ($A5:E554): the
 * figure that runs, waits and leaps, and the one that falls and poses.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorAnimByTile(void);
void abi_ActorAnimRiseFall(void);
void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorFlyFallCapped(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWall8(void);
void abi_ActorPlaceAtSpot(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXFromA5(void);
void abi_ActorSpeedYFromA5(void);
void abi_AnimStart(void);
void abi_GuardRestart(void);

#define HOP_TIME 0x42u                  /* frames left of its run */
#define SCENE_LEAPS 0x49u               /* counted up as the running figure rears to leap */
#define SCENE_TILE 0x0012u              /* PointInGround's tile behaviour: bit 5 an edge */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

static void anim(uint8_t a, uint32_t site)
{
    a8_load(a);
    jsl_call(site, abi_AnimStart);
}

static void speed_x(uint8_t y, uint16_t ret)
{
    y8_load(y);
    call_frame(ret, abi_ActorSpeedXFromA5);
}

static void speed_y(uint8_t y, uint16_t ret)
{
    y8_load(y);
    call_frame(ret, abi_ActorSpeedYFromA5);
}

/*
 * @routine A5:EB31 PlayerSceneRuns
 * @actor
 * WHAT IT DOES: The scripted figure's way 1: it stands, then runs along
 * the ground (dropping off edges as it comes to them) until it is more
 * than 192 pixels right of the player; it is put at spot $1C facing left
 * until the player is well on ($0B00), runs back 72 frames, rears and
 * jumps, waits for the player to pass, and leaps again; on landing it
 * starts over as way 0.
 * HOW: Animation 0, COP $40; animation 2, ActorSpeedXFromA5 $15,
 * ActorSpeedYFromA5 6. Each frame (BRK): ActorMoveX, ActorSnapToGround;
 * off the ground or at an edge (bit 5): gravity $38, ActorSpeedYFromA5 9,
 * each frame ActorAnimRiseFall and ActorFly until ActorOutOfGround, then
 * the run again from animation 2. Else ActorAnimByTile; the player's x +
 * $C0 below its own: ActorPlaceAtSpot $1C, animation 0, facing and flip
 * 1; BRK until the player's x high byte is $0B or more; COP $10; facing
 * and flip 0; RUN_TIME $48, animation 1, ActorSpeedXFromA5 $0C; each frame
 * ActorMoveX, ActorSnapToGround, $48 frames. $49 up; animation $11, COP
 * $40; animation $12, gravity 8, ActorSpeedYFromA5 $0F, gravity $18 once
 * falling; each frame ActorFall until ActorOutOfGround; animation $13;
 * BRK until its x + $20 is below the player's; animation $0C, gravity
 * $38, ActorSpeedXFromA5 3, ActorSpeedYFromA5 $0C; each frame
 * ActorAnimRiseFall, ActorFlyFallCapped, ActorOutOfWall8 until
 * ActorOutOfGround; animation 0, BRK; $07 0 and on at SCENE_RESTART.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 1. Exit: never returns.
 */
void abi_PlayerSceneRuns(void)
{
    uint16_t reach, x;
    uint8_t tile;

    anim(0x00, 0xA5EB33u);
    a8_load(0x40);
    actor_wait(0xA5EB39u);
    for (;;) {
        /* the run */
        anim(0x02, 0xA5EB3Du);
        speed_x(0x15, 0xEB45);
        speed_y(0x06, 0xEB4A);
        for (;;) {
            actor_yield(0xA5EB4Bu);
            jsl_call(0xA5EB4Du, abi_ActorMoveX);
            jsl_call(0xA5EB51u, abi_ActorSnapToGround);
            if (cpu.p & P_Z)
                break;                           /* off the ground */
            tile = rd8(dba(SCENE_TILE));
            a8_load(tile);
            if (tile & 0x20)
                break;                           /* at an edge */
            call_frame(0xEB60u, abi_ActorAnimByTile);
            op_rep(0x20);
            flag_set(P_C, 0);
            reach = op_adc16(rd16(dba(PLAYER_X_AT)), 0x00C0, &cpu.p);
            a16_load(reach);
            x = dp16(ACTOR_X + 1);
            cmp16_of(reach, x);
            op_sep(0x20);
            if (reach < x)
                goto ahead;
        }
        /* the drop */
        a8_load(0x38);
        wr8(dpa(ACTOR_ACCEL_Y), 0x38);
        speed_y(0x09, 0xEB8D);
        do {
            actor_yield(0xA5EB8Eu);
            call_frame(0xEB92u, abi_ActorAnimRiseFall);
            jsl_call(0xA5EB93u, abi_ActorFly);
            jsl_call(0xA5EB97u, abi_ActorOutOfGround);
        } while (cpu.p & P_Z);
    }

ahead:
    x8_load(0x1C);
    call_frame(0xEB74u, abi_ActorPlaceAtSpot);
    op_sep(0x20);
    anim(0x00, 0xA5EB79u);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    do {
        actor_yield(0xA5EB9Fu);
        a8_load(rd8(dba(PLAYER + ACTOR_X + 2)));
        cmp8_of((uint8_t)cpu.a, 0x0B);
    } while ((uint8_t)cpu.a < 0x0B);
    a8_load(0x10);
    actor_wait(0xA5EBAAu);
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
    a8_load(0x48);
    wr8(dpa(HOP_TIME), 0x48);
    anim(0x01, 0xA5EBB6u);
    speed_x(0x0C, 0xEBBE);
    do {
        actor_yield(0xA5EBBFu);
        jsl_call(0xA5EBC1u, abi_ActorMoveX);
        jsl_call(0xA5EBC5u, abi_ActorSnapToGround);
    } while (dec8_at(dpa(HOP_TIME)));

    /* rears and jumps */
    (void)inc8_at(dpa(SCENE_LEAPS));
    anim(0x11, 0xA5EBD1u);
    a8_load(0x40);
    actor_wait(0xA5EBD7u);
    anim(0x12, 0xA5EBDBu);
    a8_load(0x08);
    wr8(dpa(ACTOR_ACCEL_Y), 0x08);
    speed_y(0x0F, 0xEBE7);
    do {
        a8_load(rd8(dpa(ACTOR_SPEED_Y + 2)));
        if (!(cpu.a & 0x80)) {
            a8_load(0x18);
            wr8(dpa(ACTOR_ACCEL_Y), 0x18);
        }
        actor_yield(0xA5EBF0u);
        jsl_call(0xA5EBF2u, abi_ActorFall);
        jsl_call(0xA5EBF6u, abi_ActorOutOfGround);
    } while (cpu.p & P_Z);

    /* waits for the player to pass */
    anim(0x13, 0xA5EBFEu);
    do {
        actor_yield(0xA5EC02u);
        op_rep(0x20);
        flag_set(P_C, 0);
        reach = op_adc16(dp16(ACTOR_X + 1), 0x0020, &cpu.p);
        a16_load(reach);
        x = rd16(dba(PLAYER_X_AT));
        cmp16_of(reach, x);
        op_sep(0x20);
    } while (reach >= x);

    /* the leap */
    anim(0x0C, 0xA5EC15u);
    a8_load(0x38);
    wr8(dpa(ACTOR_ACCEL_Y), 0x38);
    speed_x(0x03, 0xEC21);
    speed_y(0x0C, 0xEC26);
    do {
        actor_yield(0xA5EC27u);
        call_frame(0xEC2Bu, abi_ActorAnimRiseFall);
        call_frame(0xEC2Eu, abi_ActorFlyFallCapped);
        call_frame(0xEC31u, abi_ActorOutOfWall8);
        jsl_call(0xA5EC32u, abi_ActorOutOfGround);
    } while (cpu.p & P_Z);
    anim(0x00, 0xA5EC3Au);
    actor_yield(0xA5EC3Eu);
    wr8(dpa(ACTOR_07), 0);
    abi_GuardRestart();
}

/*
 * @routine A5:EEE1 PlayerSceneFallPose
 * @actor
 * WHAT IT DOES: The scripted figure's way 6: it falls in pose $1C to the
 * ground, holds there, poses $1D, and after a while starts over as way 0.
 * HOW: M and X 8-bit; animation $1C; each frame (BRK) ActorFall until
 * ActorOutOfGround; COP $C0; animation $1D; COP $90, COP $80; $07 0 and
 * on at SCENE_RESTART.
 * Entry: D = the actor's record; DB = $A5; reached by the jump through
 * the table at $A5:E56D for $07 = 6. Exit: never returns.
 */
void abi_PlayerSceneFallPose(void)
{
    op_sep(0x30);
    anim(0x1C, 0xA5EEE5u);
    do {
        actor_yield(0xA5EEE9u);
        jsl_call(0xA5EEEBu, abi_ActorFall);
        jsl_call(0xA5EEEFu, abi_ActorOutOfGround);
    } while (cpu.p & P_Z);
    a8_load(0xC0);
    actor_wait(0xA5EEF7u);
    anim(0x1D, 0xA5EEFBu);
    a8_load(0x90);
    actor_wait(0xA5EF01u);
    a8_load(0x80);
    actor_wait(0xA5EF05u);
    wr8(dpa(ACTOR_07), 0);
    abi_GuardRestart();
}
