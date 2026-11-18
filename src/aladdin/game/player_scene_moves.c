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
void abi_EscortStaySpot18(void);

#define SCENE_CUE 0x146Cu  /* counted up by scripted actors as the scene goes on */
#define HOP_TIME 0x42u  /* frames left of its run */
#define SCENE_START_COUNT 0x1475u      /* the count A5:E515 kept when the figure started (the cartridge RAM's byte, plus 1) */
#define SCENE_START_SLOT 0x1476u       /* its index into the cartridge RAM's bytes */
#define SCENE_CART_RAM 0x700000u       /* cartridge RAM (the board has none: a read gives the bus's last byte) */
#define SCENE_STEP_A 0x00BDu           /* nonzero: the walk-off figure goes on (set by the scene) */
#define SCENE_STEP_B 0x00BEu           /* counted up as the walk-off figure has run past the player */
#define SCENE_STEP_C 0x00BFu           /* counted up as the walk-off figure has reached its mark */
#define SCENE_MARK_X 0x0F68u           /* the walk-off figure's mark */

/* Each frame (BRK at `brk_at` first) until the scene's cue is given. */
/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

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
    jsl_call(0xA5EC47u, abi_AnimStart);
    a8_load(0x20);
    actor_wait(0xA5EC4Du);
    /* runs till he is off the ground */
    a8_load(0x02);
    jsl_call(0xA5EC51u, abi_AnimStart);
    y8_load(0x0F);
    call_frame(0xEC59u, abi_ActorSpeedXFromA5);
    do {
        actor_yield(0xA5EC5Au);
        jsl_call(0xA5EC5Cu, abi_ActorMoveX);
        jsl_call(0xA5EC60u, abi_ActorSnapToGround);
    } while (!(cpu.p & P_Z));
    /* leaps, until the cue */
    wr8(dba(SCENE_CUE), 0);
    a8_load(0x0C);
    jsl_call(0xA5EC6Bu, abi_AnimStart);
    a8_load(0x38);
    wr8(dpa(ACTOR_ACCEL_Y), 0x38);
    y8_load(0x12);
    call_frame(0xEC77u, abi_ActorSpeedYFromA5);
    do {
        actor_yield(0xA5EC78u);
        jsl_call(0xA5EC7Au, abi_ActorFly);
        cue = rd8(dba(SCENE_CUE));
        a8_load(cue);
    } while (!cue);
    wr8(dba(SCENE_CUE), 0);
    a8_load(0x0D);
    jsl_call(0xA5EC88u, abi_AnimStart);
    wait_cue(0xA5EC8Cu);
    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);
    for (;;) {
        a8_load(0x7F);
        actor_wait(0xA5EC99u);
    }
}


/* Each frame (BRK at `brk_at` first) falling until he is in the ground. */
static void fall_to_ground(uint32_t brk_at, uint32_t fall_at, uint32_t ground_at)
{
    do {
        actor_yield(brk_at);
        jsl_call(fall_at, abi_ActorFall);
        jsl_call(ground_at, abi_ActorOutOfGround);
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
    call_frame(0xECAFu, abi_ActorSpeedYFromA5);
    fall_to_ground(0xA5ECB0u, 0xA5ECB2u, 0xA5ECB6u);
    for (;;) {                            /* a hop left, a pause, again */
        y8_load(0x06);
        call_frame(0xECC0u, abi_ActorSpeedYFromA5);
        y8_load(0x0C);
        call_frame(0xECC5u, abi_ActorSpeedXFromA5);
        a8_load(0x01);
        jsl_call(0xA5ECC8u, abi_AnimStart);
        a8_load(0x30);
        wr8(dpa(HOP_TIME), 0x30);
        do {
            actor_yield(0xA5ECD0u);
            jsl_call(0xA5ECD2u, abi_ActorMoveX);
            jsl_call(0xA5ECD6u, abi_ActorSnapToGround);
        } while (dec8_at(dpa(HOP_TIME)));
        a8_load(0x00);
        jsl_call(0xA5ECE0u, abi_AnimStart);
        a8_load(0x30);
        actor_wait(0xA5ECE6u);
        a8_load(0x0A);
        jsl_call(0xA5ECEAu, abi_AnimStart);
        a8_load(0x10);
        actor_wait(0xA5ECF0u);
        a8_load(0x00);
        jsl_call(0xA5ECF4u, abi_AnimStart);
        a8_load(0x38);
        wr8(dpa(ACTOR_ACCEL_Y), 0x38);
        y8_load(0x0C);
        call_frame(0xED00u, abi_ActorSpeedYFromA5);
        fall_to_ground(0xA5ED01u, 0xA5ED03u, 0xA5ED07u);
    }
}

/* Each frame (BRK at `brk_at`): ActorMoveX and ActorSnapToGround. */
static void walk_step(uint32_t brk_at)
{
    actor_yield(brk_at);
    jsl_call(brk_at + 2, abi_ActorMoveX);          /* BRK (2 bytes), JSL, JSL */
    jsl_call(brk_at + 6, abi_ActorSnapToGround);
}

/*
 * @routine A5:EE29 PlayerSceneWalkOff
 * @actor
 * WHAT IT DOES: The scripted figure's way 5: unless the cartridge RAM
 * still holds the count the start kept, it goes to the escort's spot
 * ($18) instead; else it falls to the ground, walks right to its mark,
 * poses, runs on until it is 24 pixels past the player, turns to face
 * left for a while, and, once the scene says so, walks on for good.
 * HOW: SCENE_START_COUNT - 1 against the cartridge RAM's byte at
 * SCENE_START_SLOT: different, on at EscortStaySpot18. Each frame
 * ActorOutOfGround zero: BRK and ActorFly. Facing and flip 0, animation 1,
 * ActorSpeedXFromA5 0; each frame (BRK) ActorMoveX, ActorSnapToGround
 * until its x (16-bit) reaches SCENE_MARK_X. SCENE_STEP_C up; animation 6,
 * COP $30; animation $1E, COP $20; animation 2, ActorSpeedXFromA5 $12;
 * each frame ActorMoveX, ActorSnapToGround while the player's x + $18 is
 * at least its own. SCENE_STEP_B up; animation 4, flip 1, COP $80; flip 0,
 * animation 0; BRK until SCENE_STEP_A; COP $10; animation 1,
 * ActorSpeedXFromA5 0; each frame ActorMoveX and ActorSnapToGround for
 * good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the jump through the table at $A5:E56D for $07 = 5. Exit: never returns.
 */
void abi_PlayerSceneWalkOff(void)
{
    uint8_t kept, ram;
    uint16_t x, reach;

    x8_load(rd8(dba(SCENE_START_SLOT)));
    kept = (uint8_t)(rd8(dba(SCENE_START_COUNT)) - 1);
    a8_load(kept);
    ram = rd8(SCENE_CART_RAM + cpu.x);
    cmp8_of(kept, ram);
    if (kept != ram) {
        abi_EscortStaySpot18();                  /* its JMP: the piece goes on there */
        return;
    }
    for (;;) {
        jsl_call(0xA5EE39u, abi_ActorOutOfGround);
        if (!(cpu.p & P_Z))
            break;
        actor_yield(0xA5EE3Fu);
        jsl_call(0xA5EE41u, abi_ActorFly);
    }
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
    a8_load(0x01);
    jsl_call(0xA5EE4Du, abi_AnimStart);
    y8_load(0x00);
    call_frame(0xEE55u, abi_ActorSpeedXFromA5);
    do {
        walk_step(0xA5EE56u);
        op_rep(0x20);
        x = dp16(ACTOR_X + 1);
        a16_load(x);
        cmp16_of(x, SCENE_MARK_X);
        op_sep(0x20);
    } while (x < SCENE_MARK_X);
    (void)inc8_at(dba(SCENE_STEP_C));
    a8_load(0x06);
    jsl_call(0xA5EE70u, abi_AnimStart);
    a8_load(0x30);
    actor_wait(0xA5EE76u);
    a8_load(0x1E);
    jsl_call(0xA5EE7Au, abi_AnimStart);
    a8_load(0x20);
    actor_wait(0xA5EE80u);
    a8_load(0x02);
    jsl_call(0xA5EE84u, abi_AnimStart);
    y8_load(0x12);
    call_frame(0xEE8Cu, abi_ActorSpeedXFromA5);
    do {
        walk_step(0xA5EE8Du);
        op_rep(0x20);
        flag_set(P_C, 0);
        reach = op_adc16(rd16(dba(PLAYER + ACTOR_X + 1)), 0x0018, &cpu.p);
        a16_load(reach);
        x = dp16(ACTOR_X + 1);
        cmp16_of(reach, x);
        op_sep(0x20);
    } while (reach >= x);
    (void)inc8_at(dba(SCENE_STEP_B));
    a8_load(0x04);
    jsl_call(0xA5EEABu, abi_AnimStart);
    a8_load(0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x80);
    actor_wait(0xA5EEB5u);
    wr8(dpa(ACTOR_FLIP), 0x00);
    a8_load(0x00);
    jsl_call(0xA5EEBBu, abi_AnimStart);
    do {
        actor_yield(0xA5EEBFu);
        a8_load(rd8(dba(SCENE_STEP_A)));
    } while (!(uint8_t)cpu.a);
    a8_load(0x10);
    actor_wait(0xA5EEC8u);
    a8_load(0x01);
    jsl_call(0xA5EECCu, abi_AnimStart);
    y8_load(0x00);
    call_frame(0xEED4u, abi_ActorSpeedXFromA5);
    for (;;)
        walk_step(0xA5EED5u);
}
