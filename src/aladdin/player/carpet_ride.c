/*
 * carpet_ride.c: Aladdin's code on the magic carpet: the ride's start (it
 * flies in from the left), its frames (the pad steers it through
 * carpet.c's routines), a wall that knocks him off, and the crash that
 * loses a life. Two rides: the one whose record has $06 = 1 and another
 * (carpet.c's "Alt" routines).
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
void abi_ActorMove(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfWalls(void);
void abi_ActorStopY(void);
void abi_AnimStep(void);
void abi_CarpetCruiseSet(void);
void abi_CarpetCruiseSetAlt(void);
void abi_CarpetCruiseX(void);
void abi_CarpetCruiseXAlt(void);
void abi_CarpetLevelOut(void);
void abi_CarpetSpeedXLimit(void);
void abi_CarpetSpeedXLimitAlt(void);
void abi_CarpetSpeedYLimit(void);
void abi_CarpetSteadyY(void);
void abi_CarpetSteerX(void);
void abi_CarpetSteerXAlt(void);
void abi_CarpetSteerY(void);
void abi_CarpetSteerYAlt(void);
void abi_CarpetTilt(void);
void abi_PlayerBoxSet(void);
void abi_PlayerEndingRide(void);
void abi_PlayerEnter8CLong(void);
void abi_PlayerFallenTest(void);
void abi_PlayerMoveStart(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);

void abi_PlayerCarpet(void);
void abi_PlayerCarpetSteer(void);
void abi_PlayerCarpetWallHurt(void);
void abi_PlayerCarpetCrash(void);
static int wall_hurt(void);

#define PAD1_DIRS 0x0380u           /* pad 1's high byte: its low 4 bits Up Down Left Right */
#define TILE_BITS 0x0012u           /* the last probed tile's behaviour (bit 5: it hurts) */
#define PLAYER_FLAGS 0x0347u        /* bit 7 cleared as the ride starts */
#define SUB_LAYERS 0x039Au          /* the layers on the sub screen: bit 4 (sprites) set */
#define LIVES 0x0364u               /* lives left */
#define SCENE_WAIT 0x0087u          /* the crash's end waits for this to be 0 */
#define SCENE_ON 0x03FAu            /* counted up as a life is lost */
#define SCROLL_KIND 0x083Du         /* the level's scroll handler, times 2 */
#define VIEW_EDGE_RULE 0x145Bu      /* the rule the view's edges follow */
#define V_1A10 0x1A10u
#define V_1A12 0x1A12u

#define FIGURE_29 0x29u             /* word: his picture list */
#define CARPET_DIRECTION 0x14u      /* the steering direction (carpet.c) */
#define CARPET_CLIMBING 0x42u       /* nonzero: tilted to climb */
#define CARPET_TILTED 0x43u         /* CarpetTilt's answer: nonzero when it tilted */
#define CARPET_SPED_UP 0x44u
#define CARPET_SLOWED 0x45u
#define FLY_IN_FRAMES ACTOR_STREAM  /* frames of the flight in */

#define CARPET_DIRECTIONS 0xC7AAu   /* (bank $80) the steering direction by the pad's low 4 bits; negative: none */

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

static void move_start(uint8_t move)
{
    a8_load(move);
    abi_PlayerMoveStart();
}

static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

static void task_exec(uint32_t site)
{
    jsl_frame(site);
    abi_TaskExec();
}

/* The ride with $06 = 1 (LDA $06, DEC A: zero). */
static int first_ride(void)
{
    uint8_t v = (uint8_t)(rd8(dpa(ACTOR_06)) - 1);

    a8_load(v);
    return v == 0;
}

/* The direction steered is 10 (neither faster nor slower), by LDA, SEC,
   CMP. */
static int steered_neither(void)
{
    uint8_t v = ldd(CARPET_DIRECTION);

    flag_set(P_C, 1);
    cmp8_of(v, 0x0A);
    return v == 0x0A;
}

/* The frame's end: the up-down steadying and steering, its direction
   cleared, the move. */
static void frame_end(int first)
{
    abi_CarpetSteadyY();
    if (first)
        abi_CarpetSteerY();
    else
        abi_CarpetSteerYAlt();
    wr8(dpa(CARPET_DIRECTION), 0);
    abi_ActorMove();
}

/*
 * @routine 81:E3F5 PlayerCarpetStart
 * @actor
 * WHAT IT DOES: The carpet ride's start (with $07 = 2 the ending's ride
 * instead): the rider's look and test box, the text blink's task on
 * level 7; he flies in from the left (127 frames, or $35 on the other
 * ride) and, but on levels 7 and $17, drifts to cruising speed until
 * the view holds still and starts the text blink's task; then the ride.
 * HOW: $07 = 2: PlayerEndingRide. ACTOR_09 $40D0, FIGURE_29 $8917,
 * CARPET_CLIMBING word 0, ACTOR_REDRAW $FF, PLAYER_BOX_WANTED $11,
 * PlayerBoxSet. Level 7 (and under $14): TaskCreate (slot 2, parameter 0,
 * TextBlinkTask). PLAYER_FLAGS bit 7 off, SUB_LAYERS bit 4 on;
 * CARPET_SPED_UP, CARPET_SLOWED, facing and flip 0; ActorStopY. Speed
 * $0003E0 and FLY_IN_FRAMES $7F ($06 = 1), else $000300 and $35;
 * PlayerMoveStart 0; each frame (BRK first) ActorMoveX for them. Speed
 * fraction 0. Not level 7 or $17: CarpetCruiseSetAlt, each frame (BRK
 * first) ActorMoveX while V_1A12 is set, TaskCreate (TextBlinkTask). On
 * at PlayerCarpet.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * PlayerStart's jump for $06 = 1-3 or $07 = 1. Exit: never returns.
 */
void abi_PlayerCarpetStart(void)
{
    uint8_t v;

    v = ldd(ACTOR_07);
    cmp8_of(v, 0x02);
    if (v == 0x02) {
        abi_PlayerEndingRide();
        return;
    }
    op_rep(P_M);
    a16_load(0x40D0);
    dp16_set(ACTOR_09, 0x40D0);
    a16_load(0x8917);
    dp16_set(FIGURE_29, 0x8917);
    dp16_set(CARPET_CLIMBING, 0);
    op_sep(P_M);
    putd(ACTOR_REDRAW, 0xFF);
    put(PLAYER_BOX_WANTED, 0x11);
    call_frame(0xE419, abi_PlayerBoxSet);
    v = ld(LEVEL);
    cmp8_of(v, 0x07);
    if (v == 0x07) {
        cmp8_of(v, 0x14);
        task_create(0x81E425u);
    }
    v = (uint8_t)(rd8(dba(PLAYER_FLAGS)) & 0x7F);
    a8_load(v);
    wr8(dba(PLAYER_FLAGS), v);
    v = (uint8_t)(rd8(dba(SUB_LAYERS)) | 0x10);
    a8_load(v);
    wr8(dba(SUB_LAYERS), v);
    wr8(dpa(CARPET_SPED_UP), 0);
    wr8(dpa(CARPET_SLOWED), 0);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    jsl_frame(0x81E446u);
    abi_ActorStopY();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (first_ride()) {
        putd(ACTOR_SPEED_X, 0xE0);
        putd(ACTOR_SPEED_X + 1, 0x03);
        wr8(dpa(ACTOR_SPEED_X + 2), 0);
        a8_load(0x7F);
    } else {
        wr8(dpa(ACTOR_SPEED_X), 0);
        putd(ACTOR_SPEED_X + 1, 0x03);
        wr8(dpa(ACTOR_SPEED_X + 2), 0);
        a8_load(0x35);
    }
    wr8(dpa(FLY_IN_FRAMES), (uint8_t)cpu.a);
    move_start(0x00);
    do {
        actor_yield(0x81E46Eu);
        abi_ActorMoveX();
    } while (dec8_at(dpa(FLY_IN_FRAMES)));
    wr8(dpa(ACTOR_SPEED_X), 0);
    v = ld(LEVEL);
    cmp8_of(v, 0x07);
    if (v != 0x07) {
        cmp8_of(v, 0x17);
        if (v != 0x17) {
            call_frame(0xE487, abi_CarpetCruiseSetAlt);
            do {
                actor_yield(0x81E488u);
                abi_ActorMoveX();
            } while (ld(V_1A12));
            task_create(0x81E493u);
        }
    }
    abi_PlayerCarpet();
}

/*
 * @routine 81:E49C PlayerCarpet
 * @actor
 * WHAT IT DOES: A frame of the carpet ride: the rider's animation, a
 * wall that hurts (on the first ride, at x under $1E00), and the pad: a
 * direction steers (PlayerCarpetSteer); none levels the carpet off and
 * drifts it back to cruising.
 * HOW: BRK; AnimStep. First ride and x's high byte under $1E:
 * PlayerCarpetWallHurt. CARPET_DIRECTIONS by PAD1_DIRS's low 4 bits: not
 * negative, it to CARPET_DIRECTION and on at PlayerCarpetSteer. Else
 * CarpetLevelOut; CarpetCruiseX (Alt), then direction 10 CarpetCruiseSet
 * (on the first ride a path no script runs), else CarpetSteerX (Alt);
 * CarpetSteadyY, CarpetSteerY (Alt); CARPET_DIRECTION 0, ActorMove; again.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
 * from PlayerCarpetStart's end and by a jump from its own frame's end
 * and PlayerCarpetSteer. Exit: never returns.
 */
void abi_PlayerCarpet(void)
{
    uint8_t v;
    int first;

    for (;;) {
        actor_yield(0x81E49Cu);
        jsl_frame(0x81E49Eu); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
        if (first_ride()) {
            v = ldd(ACTOR_X + 2);
            cmp8_of(v, 0x1E);
            if (v < 0x1E) {
                /* the JSR's return address pushed; a hurt drops it and the
                   ride goes on at the crash, in this same actor code */
                push16(0xE4AF);
                if (wall_hurt()) {
                    abi_PlayerCarpetCrash();
                    return;
                }
                cpu.s = (uint16_t)(cpu.s + 2);
            }
        }
        v = (uint8_t)(ld(PAD1_DIRS) & 0x0F);
        a8_load(v);
        y8_load(v);
        v = rd8(dba(CARPET_DIRECTIONS + v));
        a8_load(v);
        if (!(v & 0x80)) {
            wr8(dpa(CARPET_DIRECTION), v);
            abi_PlayerCarpetSteer();
            return;
        }
        call_frame(0xE4C2, abi_CarpetLevelOut);
        first = first_ride();
        if (first)
            call_frame(0xE4CA, abi_CarpetCruiseX);
        else
            call_frame(0xE4E4, abi_CarpetCruiseXAlt);
        if (steered_neither())
            abi_CarpetCruiseSet();
        else if (first)
            call_frame(0xE4D9, abi_CarpetSteerX);
        else
            call_frame(0xE4F3, abi_CarpetSteerXAlt);
        frame_end(first);
    }
}

/*
 * @routine 81:E51C PlayerCarpetSteer
 * @actor
 * WHAT IT DOES: A frame of the carpet ride steered: the carpet tilts,
 * its speed across follows the direction within its limits, and when it
 * tilted its speed up and down too; then the next frame.
 * HOW: CARPET_SPED_UP and CARPET_SLOWED 0; CarpetTilt to CARPET_TILTED.
 * CarpetSteerX and CarpetSpeedXLimit (Alt on the other ride); not tilted:
 * CarpetSteadyY, CarpetSteerY (Alt); tilted: CarpetSteerY (Alt) and
 * CarpetSpeedYLimit. CARPET_DIRECTION 0, ActorMove; on at PlayerCarpet.
 * Entry: M 8-bit, X 8-bit; D = the player's record; CARPET_DIRECTION the
 * pad's; DB = $80; reached by a jump from PlayerCarpet. Exit: never
 * returns.
 */
void abi_PlayerCarpetSteer(void)
{
    int first;

    wr8(dpa(CARPET_SPED_UP), 0);
    wr8(dpa(CARPET_SLOWED), 0);
    call_frame(0xE522, abi_CarpetTilt);
    wr8(dpa(CARPET_TILTED), (uint8_t)cpu.a);
    first = first_ride();
    if (first) {
        call_frame(0xE52C, abi_CarpetSteerX);
        call_frame(0xE52F, abi_CarpetSpeedXLimit);
    } else {
        call_frame(0xE53E, abi_CarpetSteerXAlt);
        call_frame(0xE541, abi_CarpetSpeedXLimitAlt);
    }
    if (!ldd(CARPET_TILTED)) {
        frame_end(first);
    } else {
        if (first)
            call_frame(0xE536, abi_CarpetSteerY);
        else
            call_frame(0xE548, abi_CarpetSteerYAlt);
        abi_CarpetSpeedYLimit();
        wr8(dpa(CARPET_DIRECTION), 0);
        jsl_frame(0x81E54Eu);
        abi_ActorMove();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
    abi_PlayerCarpet();
}

/* PlayerCarpetWallHurt's work: 1 when it hurt (its return address
   pulled, the crash next), else 0. */
static int wall_hurt(void)
{
    uint8_t v;

    abi_ActorOutOfWalls();
    v = ldd(ACTOR_08);
    if (v & 0x80)
        return 0;
    flag_set(P_Z, !(v & 0x10));
    if (v & 0x10)
        return 0;
    v = ld(TILE_BITS);
    flag_set(P_Z, !(v & 0x20));
    if (!(v & 0x20))
        return 0;
    abi_PlayerEnter8CLong();
    pla8();                               /* its own return dropped */
    pla8();
    return 1;
}

/*
 * @routine 81:E713 PlayerCarpetWallHurt
 * WHAT IT DOES: The carpet out of a wall in front; when the ground last
 * probed hurts, the rider is put into state $8C and the carpet crashes.
 * HOW: ActorOutOfWalls; ACTOR_08 bit 7 or bit 4 set, or TILE_BITS bit 5
 * clear: back. Else PlayerEnter8CLong, the call's return address dropped
 * (PLA PLA) and on at PlayerCarpetCrash.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from PlayerCarpet at the actor's level (81:E4AD). Exit: A low, N
 * and Z as the last test left them. Hurt: never returns.
 */
void abi_PlayerCarpetWallHurt(void)
{
    if (wall_hurt())
        actor_leave(0x81E7E9u);           /* PlayerCarpetCrash */
}

/*
 * @routine 81:E7E9 PlayerCarpetCrash
 * @actor
 * WHAT IT DOES: The carpet crashes: the view holds still, Aladdin loses
 * his health with the song and the hurt sound and falls away (move 5)
 * until he is out of the map; then the stage starts again once the scene
 * is over, or ends with no lives.
 * HOW: SCROLL_KIND 0, VIEW_EDGE_RULE 4, V_1A10 and V_1A12 1; facing and
 * flip 0; HEALTH 0, SCENE_ON up; SongPlay $2A, SoundQueueAdd 2; Y = $52,
 * ActorMotionPreset; PlayerMoveStart 5. Each frame (BRK first) ActorFly
 * while PlayerFallenTest's C is set. TaskCreate (slot 1, parameter 4,
 * BrightnessDownTask); frames until SCENE_WAIT is 0; LIVES down; TaskExec
 * to LevelPlayStartTask, or with none left to the task's entry for no
 * lives.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * a jump from PlayerCarpetWallHurt. Exit: never returns.
 */
void abi_PlayerCarpetCrash(void)
{
    put(SCROLL_KIND, 0x00);
    put(VIEW_EDGE_RULE, 0x04);
    put(V_1A10, 0x01);
    wr8(dba(V_1A12), 0x01);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dba(HEALTH), 0);
    (void)inc8_at(dba(SCENE_ON));
    a8_load(0x2A);
    jsl_frame(0x81E807u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x02);
    jsl_frame(0x81E80Du);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(0x52);
    jsl_frame(0x81E813u);
    abi_ActorMotionPreset();
    cpu.s = (uint16_t)(cpu.s + 3);
    move_start(0x05);
    do {
        actor_yield(0x81E81Cu);
        jsl_frame(0x81E81Eu);
        abi_ActorFly();
        cpu.s = (uint16_t)(cpu.s + 3);
        call_frame(0xE824, abi_PlayerFallenTest);
    } while (cpu.p & P_C);
    task_create(0x81E827u);
    do
        actor_yield(0x81E830u);
    while (ld(SCENE_WAIT));
    (void)dec8_at(dba(LIVES));
    if (ld(LIVES))
        task_exec(0x81E83Fu);
    else
        task_exec(0x81E846u);
}
