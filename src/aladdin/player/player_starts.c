/*
 * player_starts.c: Aladdin's code as his record starts: by its $06 the
 * carpet, else by its $07 the level's own start (a walk in, then the
 * level's tasks) or one of the scenes that place him (dropped in, a new
 * game, the story, walking on in the ending's scenes).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "wram_state.h"
#include "wram_player.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpawnShared(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedXSet(void);
void abi_PlayerBoxSet(void);
void abi_PlayerJumpPoseStep(void);
void abi_PlayerMoveStart(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);
void abi_PlayerStand(void);
void abi_PlayerCarpetStart(void);
void abi_PlayerSceneLand(void);
void abi_PlayerEndingRide(void);
void abi_PlayerSceneStand(void);

void abi_PlayerStartTasks(void);
void abi_PlayerStartDropped(void);
void abi_PlayerStartNewGame(void);
void abi_PlayerStory(void);
void abi_PlayerSceneWalkOn(void);
void abi_PlayerSceneWalkRight(void);

#define CAMERA_AIM_X 0x03F1u        /* added to the scroll x for the point the camera keeps him from (word) */
#define PLAYER_FLAGS 0x0347u        /* bit 7 cleared as he starts */
#define V_0360 0x0360u              /* nonzero: no walk in */
#define APPLES 0x0369u              /* apples left, two decimal digits */
#define LIVES 0x0364u               /* lives left */
#define V_036B 0x036Bu
#define HOVER_ON 0x036Eu            /* nonzero: the fourth action makes him hover */
#define VIEW_EDGE_RULE 0x145Bu      /* the rule the view's edges follow */
#define V_147B 0x147Bu
#define STORY_78 0x0078u            /* the story's figures: nonzero once one is in place */
#define STORY_79 0x0079u            /* nonzero at the end of the story's third part */
#define STORY_7A 0x007Au            /* set to $80: the story's parts go on */
#define CUE_CF 0x00CFu              /* nonzero while the story's text is busy */
#define CUE_BE 0x00BEu              /* the scene's cues, counted up in turn */
#define CUE_BF 0x00BFu
#define CUE_C3 0x00C3u
#define CUE_C4 0x00C4u
#define FIGURE_29 0x29u             /* word: his picture list */
#define WALK_FRAMES ACTOR_STREAM    /* frames of the walk in */

#define START_BY_06 0x81CB59u       /* the start's code by $06 (words, this bank) */
#define START_BY_07 0x81CB61u       /* by $07 */

static void move_start(uint8_t move)
{
    a8_load(move);
    abi_PlayerMoveStart();
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

static void put(uint16_t addr, uint8_t n)
{
    a8_load(n);
    wr8(dba(addr), n);
}

static void putd(uint8_t off, uint8_t n)
{
    a8_load(n);
    wr8(dpa(off), n);
}

static void wait_frames(uint8_t n, uint32_t cop_at)
{
    a8_load(n);
    actor_wait(cop_at);
}

static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* A byte of the record times 2, as LDA, ASL, TAX leave it. */
static uint8_t index_of(uint8_t off)
{
    uint8_t v = rd8(dpa(off)), i = (uint8_t)(v << 1);

    a8_load(i);
    flag_set(P_C, v & 0x80);
    x8_load(i);
    return i;
}

/* Standing still, his start's move, and on at PlayerStand. */
static void stand(void)
{
    abi_ActorSpeedXClear();
    move_start(0x00);
    abi_PlayerStand();
}

/* The level's own start ($07 = 0). */
static void level_start(void)
{
    uint8_t v;

    op_rep(P_M);
    a16_load(0x40D0);
    dp16_set(ACTOR_09, 0x40D0);
    a16_load(0x8907);
    dp16_set(FIGURE_29, 0x8907);
    op_sep(P_M);
    putd(ACTOR_REDRAW, 0xFF);
    v = (uint8_t)(rd8(dba(PLAYER_FLAGS)) & 0x7F);
    a8_load(v);
    wr8(dba(PLAYER_FLAGS), v);
    if (ld(V_0360)) {
        abi_PlayerStartTasks();
        return;
    }
    v = ld(LEVEL);
    cmp8_of(v, 0x14);
    if (v >= 0x14) {
        cmp8_of(v, 0x1A);
        if (v == 0x1A) {
            /* no script runs this path */
            a8_load(0x60);
            abi_ActorSpawnShared();
        }
        stand();
        return;
    }
    /* the walk in: 32 frames ($68 on level 6, faster) */
    move_start(0x02);
    putd(WALK_FRAMES, 0x20);
    y8_load(0x03);
    v = ld(LEVEL);
    cmp8_of(v, 0x06);
    if (v == 0x06) {
        putd(WALK_FRAMES, 0x68);
        y8_load(0x6C);
    }
    abi_ActorSpeedXSet();
    do {
        actor_yield(0x81CBC3u);
        abi_ActorMoveX();
        abi_ActorSnapToGround();
    } while (dec8_at(dpa(WALK_FRAMES)));
    abi_PlayerStartTasks();
}





/* A turn to the left and back: move 5 for 8 frames, then move 0. */
static void turn(uint32_t cop_at)
{
    move_start(0x05);
    wait_frames(0x08, cop_at);
    move_start(0x00);
}

static void yield_while_text(uint32_t brk_at)
{
    do
        actor_yield(brk_at);
    while (ld(CUE_CF));
}

/*
 * @routine 81:DE79 PlayerStory
 * @actor
 * WHAT IT DOES: Aladdin in the story's scene: he turns this way and that
 * as its figures come and its pages of text go, starts the story's next
 * scenes, and at its end is shown no more.
 * HOW: FIGURE_29 $8907; PLAYER_BOX_WANTED and V_142D 0; facing and flip
 * 1; PlayerMoveStart 0; ACTOR_09 $D0, ACTOR_REDRAW $FF. Frames (BRK)
 * until STORY_78; a turn (move 5, COP 8, move 0), flip 0; frames until
 * CUE_CF is 0; COP $20, a turn, flip up, COP $20, TaskCreate (slot 4,
 * parameter 9, STORY_SCENE_TASK); frames until CUE_CF is 0; COP $20, a
 * turn, flip 0, TaskCreate (slot 4, parameter $0B); frames until CUE_CF
 * is 0; COP $10, STORY_7A $80; frames until STORY_79; a turn, flip up;
 * frames while TASK_PARAM is 4; ACTOR_09 bits 6 and 7 cleared (TRB), x's
 * high byte 1; then BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 7. Exit: never returns.
 */
void abi_PlayerStory(void)
{
    uint8_t v;

    op_rep(P_M);
    a16_load(0x8907);
    dp16_set(FIGURE_29, 0x8907);
    op_sep(P_M);
    wr8(dba(PLAYER_BOX_WANTED), 0);
    wr8(dba(V_142D), 0);
    putd(ACTOR_FACING, 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    move_start(0x00);
    putd(ACTOR_09, 0xD0);
    putd(ACTOR_REDRAW, 0xFF);
    do
        actor_yield(0x81DE9Bu);
    while (!ld(STORY_78));
    turn(0x81DEA9u);
    wr8(dpa(ACTOR_FLIP), 0);
    yield_while_text(0x81DEB2u);
    wait_frames(0x20, 0x81DEBBu);
    turn(0x81DEC4u);
    (void)inc8_at(dpa(ACTOR_FLIP));
    wait_frames(0x20, 0x81DECFu);
    task_create(0x81DED1u);
    yield_while_text(0x81DEDAu);
    wait_frames(0x20, 0x81DEE3u);
    turn(0x81DEECu);
    wr8(dpa(ACTOR_FLIP), 0);
    task_create(0x81DEF5u);
    yield_while_text(0x81DEFEu);
    wait_frames(0x10, 0x81DF07u);
    put(STORY_7A, 0x80);
    do
        actor_yield(0x81DF0Eu);
    while (!ld(STORY_79));
    turn(0x81DF1Cu);
    (void)inc8_at(dpa(ACTOR_FLIP));
    do {
        actor_yield(0x81DF25u);
        v = ld(TASK_PARAM);
        cmp8_of(v, 0x04);
    } while (v == 0x04);
    a8_load(0xC0);
    trb_at(dpa(ACTOR_09), 0xC0);
    putd(ACTOR_X + 2, 0x01);
    for (;;)
        actor_yield(0x81DF36u);
}

/*
 * @routine 81:E042 PlayerSceneWalkOn
 * @actor
 * WHAT IT DOES: Aladdin in a scene of level 5's end: he drops to the
 * ground if he is above it, faces right and cues the scene, walks right
 * to x $70, stops, and cues it on twice with a pose ($1B) between; then
 * he stays.
 * HOW: ActorSnapToGround Z set: PlayerMoveStart $0E, vertical speed
 * $000300, each frame (BRK first) ActorMoveY until ActorOutOfGround Z
 * clear. Facing and flip 0, PlayerMoveStart 0; CUE_C3 and V_147B up; COP
 * $40; CUE_C4 up; PlayerMoveStart 2, speed $000100; each frame (BRK
 * first) ActorMoveX until x pixels' low byte is $70 or more;
 * PlayerMoveStart 0, COP $20, CUE_BE up, PlayerMoveStart $1B, COP $A0,
 * CUE_BF up; then COP $80 for good.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $07 = 10. Exit: never returns.
 */
void abi_PlayerSceneWalkOn(void)
{
    uint8_t v;

    abi_ActorSnapToGround();
    if (cpu.p & P_Z) {
        move_start(0x0E);
        putd(ACTOR_SPEED_Y + 1, 0x03);
        wr8(dpa(ACTOR_SPEED_Y), 0);
        wr8(dpa(ACTOR_SPEED_Y + 2), 0);
        do {
            actor_yield(0x81E055u);
            abi_ActorMoveY();
            abi_ActorOutOfGround();
        } while (cpu.p & P_Z);
    }
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    move_start(0x00);
    (void)inc8_at(dba(CUE_C3));
    (void)inc8_at(dba(V_147B));
    wait_frames(0x40, 0x81E072u);
    (void)inc8_at(dba(CUE_C4));
    move_start(0x02);
    putd(ACTOR_SPEED_X + 1, 0x01);
    wr8(dpa(ACTOR_SPEED_X), 0);
    wr8(dpa(ACTOR_SPEED_X + 2), 0);
    do {
        actor_yield(0x81E084u);
        abi_ActorMoveX();
        v = ldd(ACTOR_X + 1);
        cmp8_of(v, 0x70);
    } while (v < 0x70);
    move_start(0x00);
    wait_frames(0x20, 0x81E097u);
    (void)inc8_at(dba(CUE_BE));
    move_start(0x1B);
    wait_frames(0xA0, 0x81E0A3u);
    (void)inc8_at(dba(CUE_BF));
    for (;;)
        wait_frames(0x80, 0x81E0AAu);
}

