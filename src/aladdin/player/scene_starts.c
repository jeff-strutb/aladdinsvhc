/*
 * scene_starts.c: Aladdin's code for the scene starts in PlayerStart's
 * table by $07 that no level plays: placed and waiting, then walking right
 * ($07 2-4); walking in and jumping down ($07 8); running to the middle and
 * on through the scene's cues ($07 12); walking out to the next act ($07
 * 13).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_PlayerMoveStart(void);
void abi_ActorSpeedXSet(void);
void abi_ActorMoveX(void);
void abi_ActorMotionPreset(void);
void abi_PlayerJumpPoseStep(void);
void abi_ActorFly(void);
void abi_ActorOutOfGround(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF6With1(void);
void abi_PlayerStand(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_PlayerSceneAnimStart(void);
void abi_PlayerSceneMoveStep(void);
void abi_PlayerSceneAnimStep(void);
void abi_PlayerSceneWaitWalk(void);

#define FIGURE_29 0x29u            /* word: his picture list */
#define ACTOR_28 0x28u
#define PFW_SCENE 0x1430u          /* nonzero: a scene moves the player */
#define TASK_PARAM 0x0075u         /* the first task slot's parameter */
#define TASK2_STATE 0x009Fu        /* task slot 2's state (0 once it has ended) */
#define CUE_BE 0x00BEu             /* the scene's cue (bit 7: on) */
#define VIEW_EDGE_RULE 0x145Bu     /* the rule the view's edges follow */
#define V_147B 0x147Bu
#define V_0978 0x0978u
#define V_0360 0x0360u             /* the restart point */
#define PART 0x035Cu               /* the story's part */
#define V_035D 0x035Du
#define CUE_1B31 0x1B31u           /* the scene's cues for $07 = 12 */
#define CUE_1B32 0x1B32u
#define V_141B 0x141Bu
#define V_141C 0x141Cu
#define LANDED 0x1428u             /* nonzero once he is on the ground */
#define V_03FB 0x03FBu

/* A JSL at `site` from C: its return address on the stack while the
   callee runs, as the original's. */
static void jsl_at(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* PlayerMoveStart by JSR, its last byte at ret. */
static void move_start(uint8_t move, uint16_t ret)
{
    a8_load(move);
    call_frame(ret, abi_PlayerMoveStart);
}

/* A JSR (its last byte at ret) to one of the scene steps, which put their
   own JSLs' returns on the stack below it. */
static void scene_step(uint16_t ret, void (*fn)(void))
{
    call_frame(ret, fn);
}

static void anim_start(uint8_t anim, uint16_t ret)
{
    a8_load(anim);
    scene_step(ret, abi_PlayerSceneAnimStart);
}

static uint8_t ld(uint16_t addr)
{
    uint8_t v = rd8(dba(addr));

    a8_load(v);
    return v;
}

static uint8_t ldd(uint8_t off)
{
    uint8_t v = rd8(dpa(off));

    a8_load(v);
    return v;
}

static void putd(uint8_t off, uint8_t n)
{
    a8_load(n);
    wr8(dpa(off), n);
}

/* LDA dp, CMP #n: C set when the byte is n or more. */
static int at_least(uint8_t off, uint8_t n)
{
    uint8_t v = ldd(off);

    cmp8_of(v, n);
    return v >= n;
}

/* PlaceHigh and PlaceLow: placed at x, y, facing right, standing. */
static void place(uint16_t x, uint16_t y, uint16_t ret)
{
    op_rep(P_M);
    a16_load(0x8907);
    wr16(dpa(FIGURE_29), 0x8907);
    a16_load(x);
    wr16(dpa(ACTOR_X + 1), x);
    a16_load(y);
    wr16(dpa(ACTOR_Y + 1), y);
    op_sep(P_M);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    move_start(0x00, ret);
    abi_PlayerSceneWaitWalk();
}

/*
 * @routine 81:DDC1 PlayerScenePlaceHigh
 * @actor
 * WHAT IT DOES: The player started with $07 = 2 or 3: placed at x $0030,
 * y $0048, facing right and standing; on at PlayerSceneWaitWalk.
 * HOW: (16-bit A) FIGURE_29 $8907, x pixels $0030, y pixels $0048;
 * facing and flip 0; PlayerMoveStart 0; on at PlayerSceneWaitWalk.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 2 or 3. Exit: never returns.
 */
void abi_PlayerScenePlaceHigh(void)
{
    place(0x0030, 0x0048, 0xDDDC);
}

/*
 * @routine 81:DDE0 PlayerScenePlaceLow
 * @actor
 * WHAT IT DOES: The player started with $07 = 4: placed at x $0020, y
 * $00A8, facing right and standing; on at PlayerSceneWaitWalk.
 * HOW: As PlayerScenePlaceHigh, with x $0020 and y $00A8.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 4. Exit: never returns.
 */
void abi_PlayerScenePlaceLow(void)
{
    place(0x0020, 0x00A8, 0xDDFB);
}

/*
 * @routine 81:DF3A PlayerSceneWaitWalk
 * @actor
 * WHAT IT DOES: Aladdin placed in a scene waits until the first task's
 * parameter is set, stands 16 frames more, then walks right for ever.
 * HOW: PFW_SCENE up; ACTOR_09 $F0, ACTOR_28 $FF, ACTOR_06 0, TASK_PARAM
 * 0; each frame (BRK first) until TASK_PARAM is nonzero; PlayerMoveStart
 * 0, COP $10; PlayerMoveStart 3; flip 0, speed $0200 (its fraction 0);
 * each frame (BRK first) ActorMoveX.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerScenePlaceHigh and PlayerScenePlaceLow. Exit: never
 * returns.
 */
void abi_PlayerSceneWaitWalk(void)
{
    inc8_at(dba(PFW_SCENE));
    putd(ACTOR_09, 0xF0);
    putd(ACTOR_28, 0xFF);
    wr8(dpa(ACTOR_06), 0);
    wr8(dba(TASK_PARAM), 0);
    do
        actor_yield(0x81DF4Au);
    while (!ld(TASK_PARAM));
    move_start(0x00, 0xDF55);
    a8_load(0x10);
    actor_wait(0x81DF58u);
    move_start(0x03, 0xDF5E);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dpa(ACTOR_SPEED_X), 0);
    putd(ACTOR_SPEED_X + 1, 0x02);
    for (;;) {
        actor_yield(0x81DF67u);
        jsl_at(0x81DF69u, abi_ActorMoveX);
    }
}

/*
 * @routine 81:DF6F PlayerSceneJumpIn
 * @actor
 * WHAT IT DOES: The player started with $07 = 8: Aladdin walks right for
 * 24 frames, jumps, lands, and after his landing's pose stands.
 * HOW: Facing and flip 0; PlayerMoveStart 2; Y = 3, ActorSpeedXSet; 24
 * frames (BRK first) of ActorMoveX; Y = $4D, ActorMotionPreset; the pose
 * step and timer 0; each frame (BRK first) PlayerJumpPoseStep, ActorFly,
 * until he falls (vertical speed not negative) and ActorOutOfGround leaves
 * Z clear; SoundQueueAdd $10; PlayerMoveStart $19, COP $80; on at
 * PlayerStand.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 8. Exit: never returns.
 */
void abi_PlayerSceneJumpIn(void)
{
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    move_start(0x02, 0xDF77);
    y8_load(0x03);
    jsl_at(0x81DF7Au, abi_ActorSpeedXSet);
    putd(ACTOR_STREAM, 0x18);
    do {
        actor_yield(0x81DF82u);
        jsl_at(0x81DF84u, abi_ActorMoveX);
    } while (dec8_at(dpa(ACTOR_STREAM)));
    y8_load(0x4D);
    jsl_at(0x81DF8Eu, abi_ActorMotionPreset);
    wr8(dpa(ACTOR_40), 0);
    wr8(dpa(ACTOR_41), 0);
    for (;;) {
        actor_yield(0x81DF96u);
        call_frame(0xDF9A, abi_PlayerJumpPoseStep);
        jsl_at(0x81DF9Bu, abi_ActorFly);
        if (ldd(ACTOR_SPEED_Y + 2) & 0x80)
            continue;
        jsl_at(0x81DFA3u, abi_ActorOutOfGround);
        if (!(cpu.p & P_Z))
            break;
    }
    a8_load(0x10);
    jsl_at(0x81DFABu, abi_SoundQueueAdd);
    move_start(0x19, 0xDFB3);
    a8_load(0x80);
    actor_wait(0x81DFB6u);
    abi_PlayerStand();
}

/*
 * @routine 81:E0AE PlayerSceneWalkOut
 * @actor
 * WHAT IT DOES: The player started with $07 = 13: Aladdin walks right to
 * the scene's cue point, waits for the cue, walks on as the screen fades,
 * and the story goes on to its next part (the task at $81:F61D).
 * HOW: Facing and flip 0; VIEW_EDGE_RULE 4, V_147B up; PlayerMoveStart 2,
 * speed $0100; each frame (BRK first) ActorMoveX until x pixels' high
 * byte is 6 or more and its low byte $50 or more; PlayerMoveStart 0,
 * CUE_BE up; frames (BRK first) until CUE_BE bit 7; PlayerMoveStart 2,
 * ACTOR_40 0, V_0978 3; 256 frames (BRK first) of ActorMoveX;
 * SoundQueueF6With1; TaskCreate (slot 2, parameter 4,
 * BrightnessDownTask); frames (BRK first) of ActorMoveX while slot 2's
 * state is nonzero; V_0360 and V_035D 0, PART up; TaskExec $81:F61D.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 13. Exit: never returns (TaskExec).
 */
void abi_PlayerSceneWalkOut(void)
{
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    a8_load(0x04);
    wr8(dba(VIEW_EDGE_RULE), 0x04);
    inc8_at(dba(V_147B));
    move_start(0x02, 0xE0BE);
    putd(ACTOR_SPEED_X + 1, 0x01);
    wr8(dpa(ACTOR_SPEED_X), 0);
    wr8(dpa(ACTOR_SPEED_X + 2), 0);
    do {
        actor_yield(0x81E0C7u);
        jsl_at(0x81E0C9u, abi_ActorMoveX);
    } while (!at_least(ACTOR_X + 2, 0x06) || !at_least(ACTOR_X + 1, 0x50));
    move_start(0x00, 0xE0DD);
    inc8_at(dba(CUE_BE));
    do
        actor_yield(0x81E0E1u);
    while (!(ld(CUE_BE) & 0x80));
    move_start(0x02, 0xE0EC);
    wr8(dpa(ACTOR_40), 0);
    a8_load(0x03);
    wr8(dba(V_0978), 0x03);
    do {
        actor_yield(0x81E0F4u);
        jsl_at(0x81E0F6u, abi_ActorMoveX);
    } while (dec8_at(dpa(ACTOR_40)));
    jsl_at(0x81E0FEu, abi_SoundQueueF6With1);
    jsl_frame(0x81E102u);
    abi_TaskCreate();
    cpu.unwound = 0;
    do {
        actor_yield(0x81E10Bu);
        jsl_at(0x81E10Du, abi_ActorMoveX);
    } while (ld(TASK2_STATE));
    wr8(dba(V_0360), 0);
    wr8(dba(V_035D), 0);
    inc8_at(dba(PART));
    jsl_frame(0x81E11Fu);
    abi_TaskExec();
}

/*
 * @routine 81:E154 PlayerSceneCued
 * @actor
 * WHAT IT DOES: The player started with $07 = 12: Aladdin runs to x $80 in
 * a scene, then goes through its cues: a pose, a run to the right, a jump
 * with the hurt sound, and a last pose once he lands, for ever.
 * HOW: PlayerSceneAnimStart 0; Y = 3, ActorSpeedXSet; V_141C, V_141B 0.
 * Left of x $80 (its low byte): facing right, frames (BRK first) of
 * PlayerSceneMoveStep until it is $80 or more; else facing left, until it
 * is below. x low byte $80, flip 0, PlayerSceneAnimStart $13; frames (BRK
 * first) until CUE_1B31; PlayerSceneAnimStart $1C, 8 frames (BRK first) of
 * PlayerSceneAnimStep; V_147B up, COP $70; PlayerSceneAnimStart 1, facing
 * left; Y = $6F, ActorSpeedXSet; frames (BRK first) of ActorMoveX and
 * PlayerSceneAnimStep until CUE_1B32, then 6 more; SoundQueueAdd 2, V_03FB
 * 8; PlayerSceneAnimStart $0E; facing left, flip 0, V_141C 1, LANDED 0; Y =
 * 5, ActorMotionPreset; frames (BRK first) of PlayerSceneAnimStep and
 * ActorFly until LANDED; PlayerSceneAnimStart $0F; each frame (BRK first)
 * PlayerSceneAnimStep.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 12. Exit: never returns.
 */
void abi_PlayerSceneCued(void)
{
    anim_start(0x00, 0xE158);
    y8_load(0x03);
    jsl_at(0x81E15Bu, abi_ActorSpeedXSet);
    wr8(dba(V_141C), 0);
    wr8(dba(V_141B), 0);
    if (!at_least(ACTOR_X + 1, 0x80)) {
        wr8(dpa(ACTOR_FLIP), 0);
        wr8(dpa(ACTOR_FACING), 0);
        do {
            actor_yield(0x81E16Fu);
            scene_step(0xE173, abi_PlayerSceneMoveStep);
        } while (!at_least(ACTOR_X + 1, 0x80));
    } else {
        putd(ACTOR_FLIP, 0x01);
        wr8(dpa(ACTOR_FACING), 0x01);
        do {
            actor_yield(0x81E182u);
            scene_step(0xE186, abi_PlayerSceneMoveStep);
        } while (at_least(ACTOR_X + 1, 0x80));
    }
    putd(ACTOR_X + 1, 0x80);
    wr8(dpa(ACTOR_FLIP), 0);
    anim_start(0x13, 0xE197);
    do
        actor_yield(0x81E198u);
    while (!ld(CUE_1B31));
    anim_start(0x1C, 0xE1A3);
    putd(ACTOR_40, 0x08);
    do {
        actor_yield(0x81E1A8u);
        scene_step(0xE1AC, abi_PlayerSceneAnimStep);
    } while (dec8_at(dpa(ACTOR_40)));
    inc8_at(dba(V_147B));
    a8_load(0x70);
    actor_wait(0x81E1B6u);
    anim_start(0x01, 0xE1BC);
    putd(ACTOR_FLIP, 0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    y8_load(0x6F);
    jsl_at(0x81E1C5u, abi_ActorSpeedXSet);
    do {
        actor_yield(0x81E1C9u);
        jsl_at(0x81E1CBu, abi_ActorMoveX);
        scene_step(0xE1D1, abi_PlayerSceneAnimStep);
    } while (!ld(CUE_1B32));
    putd(ACTOR_40, 0x06);
    do {
        actor_yield(0x81E1DBu);
        jsl_at(0x81E1DDu, abi_ActorMoveX);
        scene_step(0xE1E3, abi_PlayerSceneAnimStep);
    } while (dec8_at(dpa(ACTOR_40)));
    a8_load(0x02);
    jsl_at(0x81E1EAu, abi_SoundQueueAdd);
    a8_load(0x08);
    wr8(dba(V_03FB), 0x08);
    anim_start(0x0E, 0xE1F7);
    putd(ACTOR_FACING, 0x01);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dba(V_141C), 0x01);
    wr8(dba(LANDED), 0);
    y8_load(0x05);
    jsl_at(0x81E206u, abi_ActorMotionPreset);
    do {
        actor_yield(0x81E20Au);
        scene_step(0xE20E, abi_PlayerSceneAnimStep);
        jsl_at(0x81E20Fu, abi_ActorFly);
    } while (!ld(LANDED));
    anim_start(0x0F, 0xE21C);
    for (;;) {
        actor_yield(0x81E21Du);
        scene_step(0xE221, abi_PlayerSceneAnimStep);
    }
}
