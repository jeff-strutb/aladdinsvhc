/*
 * kind28.c: actor kind $28, which waits for Aladdin to come near, rises out
 * of hiding, moves up, then drifts across bobbing on a wave; its variant
 * with bit 4 of +$07 is instead a spawner of its own kind while he is
 * near. Its second state (downed) plays out and ends it; and its wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t steps;              /* +$40: frames left of its rise */
    uint8_t wave;               /* +$41: its place in the wave (0 to $23) */
    uint8_t frames;             /* +$42: the downed state's frames left */
    uint8_t spawn_wait[2];      /* +$43: the spawner: frames before the next (word) */
    uint8_t up;                 /* +$45: set once it has risen (its wait then stomps or hurts) */
    uint8_t right;              /* +$46: counted each frame it waits facing right */
    uint8_t delay[2];           /* +$47: frames before it rises (ActorFallStart, ActorRiseAfterDelay; word) */
} Kind28Own;

_Static_assert(sizeof(Kind28Own) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind28Own *own(Actor *a) { return (Kind28Own *)a->own.bytes; }
static uint16_t spawn_wait(Actor *a) { return (uint16_t)(own(a)->spawn_wait[0] | own(a)->spawn_wait[1] << 8); }
static void set_spawn_wait(Actor *a, uint16_t v) { own(a)->spawn_wait[0] = (uint8_t)v; own(a)->spawn_wait[1] = (uint8_t)(v >> 8); }

#define NEAR_BOX 0x28           /* ActorNearPlayer's box for "near" */
#define WAVE_STEPS 0x24

enum {
    HIDDEN = ACTOR_RESUME_OWN,  /* each frame until he is near */
    SHOWN,                      /* after its 32 frames facing him */
    RISE,                       /* each frame of its rise */
    RISEN,                      /* after its 8 frames at the top */
    DRIFT,                      /* each frame of its drift, no wave */
    WAVE,                       /* each frame of its drift on the wave */
    SPAWN_COUNT,                /* the spawner: each frame of its count, or until he is near */
    SPAWN_NEAR,                 /* each frame after a spawn while he stays near */
    DOWNED,                     /* the second state: each of its 32 frames */
};

/*
 * @routine 83:A167 ActorFallStart
 * WHAT IT DOES: Starts the actor falling from rest: no speed down, gravity
 * 8, and its delay before rising (+$47-$48) $0128. Kind $28 is its only
 * caller (actor_fields.c's record; written here, the kind's own piece).
 */
static void fall_start(Actor *a)
{
    a->speed_y = 0;
    a->accel_y = 0x08;
    own(a)->delay[0] = 0x28;
    own(a)->delay[1] = 0x01;
}

/*
 * @routine 83:A17A ActorRiseAfterDelay
 * WHAT IT DOES: Counts its delay (+$47, a word) down; once it is out, the
 * actor moves up by its speed down (ActorStepYBack), which grows by its
 * gravity while its whole part is below 2. Kind $28 is its only caller
 * (actor_steps.c's record; written here, the kind's own piece).
 */
static void rise_after_delay(Actor *a)
{
    uint16_t d = (uint16_t)(own(a)->delay[0] | own(a)->delay[1] << 8);

    if (d) {
        d--;
        own(a)->delay[0] = (uint8_t)d;
        own(a)->delay[1] = (uint8_t)(d >> 8);
        return;
    }
    ActorStepYBack(a);
    if ((uint8_t)((uint32_t)a->speed_y >> 8) < 0x02)
        ActorAccelerateY(a);
}

/* the spawner's spawn: a copy of its kind and look, counted live; then
   $160 frames */
static int spawn(Actor *a)
{
    if (ActorSpawnSameLook(a, a->kind))
        g.kind_count[a->kind]++;
    set_spawn_wait(a, 0x0160);
    a->resume = SPAWN_NEAR;
    return 1;
}

/* shown: pose 1, facing him (bit 2 of +$07: by its x's bit 0), its wait
   routine on, 32 frames */
static int shown(Actor *a)
{
    AnimStart(a, 0x01);
    a->facing = (a->spawn_byte & 0x04) ? (uint8_t)(pixels(a->x) & 0x01) : ActorSideOfPlayer(a);
    a->flip = (uint8_t)(a->facing ^ 0x01);
    a->flags |= 0x80;
    a->resume = SHOWN;
    return 0x20;
}

/*
 * @routine 83:A03E Kind28Start
 * WHAT IT DOES: Actor kind $28: hidden until Aladdin is near (box $28), it
 * shows itself facing him, waits 32 frames, comes up 16 pixels (its SBC of
 * 15 runs with the carry AnimStart left clear: the picture's place plus
 * $2000 never wraps, actor_pictures being $6000 long) and rises for a
 * while (1 frame, or $2A with bit 0 of +$07), waits 8 frames, then falls
 * and drifts across on a wave (or, with bit 1 of +$07, with no wave; no
 * script has that) for good. Bit 3 of +$07: shown at once (no script);
 * level $0C: its sprite priority bits cleared (no script). With bit 4 of
 * +$07 it is a spawner instead: while he is near, a copy every $160
 * frames (no script has it). Its second entry is Kind28Second.
 */
int Kind28Start(Actor *a)
{
    Kind28Own *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        o->right = 0;
        if (a->spawn_byte & 0x10) {
            a->spawn_byte = (uint8_t)((a->spawn_byte & ~0x10) | 0x08);
            set_spawn_wait(a, 0);
            a->spawn_at = 0;
            a->resume = SPAWN_COUNT;
            return 1;
        }
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        if ((uint8_t)g.level == 0x0C)
            a->flags &= (uint8_t)~0x30;
        o->up = 0;
        if (a->spawn_byte & 0x08)
            return shown(a);
        a->resume = HIDDEN;
        return 1;
    case ACTOR_START_SECOND:
        return Kind28Second(a);
    case HIDDEN:
        if (!ActorNearPlayer(a, NEAR_BOX))
            return 1;
        return shown(a);
    case SHOWN: {
        int long_rise = a->spawn_byte & 0x01;

        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 0x10));
        AnimStart(a, 0x00);
        a->spawn_byte >>= 1;
        o->steps = long_rise ? 0x2A : 0x01;
        ActorMotionPreset(a, 0x44);
        a->resume = RISE;
        return 1;
    }
    case RISE:
        ActorStepY(a);
        if (--o->steps)
            return 1;
        o->up++;
        a->resume = RISEN;
        return 8;
    case RISEN: {
        int no_wave = a->spawn_byte & 0x01;

        a->spawn_byte >>= 1;
        fall_start(a);
        if (no_wave) {
            a->resume = DRIFT;
            return 1;
        }
        o->wave = 0x0A;
        a->resume = WAVE;
        return 1;
    }
    case DRIFT:
        ActorMoveX(a);
        rise_after_delay(a);
        return 1;
    case WAVE:
        ActorMoveX(a);
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + (int8_t)k28_wave(o->wave)));
        if (++o->wave == WAVE_STEPS)
            o->wave = 0;
        rise_after_delay(a);
        return 1;
    case SPAWN_COUNT:
        if (spawn_wait(a)) {
            set_spawn_wait(a, (uint16_t)(spawn_wait(a) - 1));
            return 1;
        }
        if (!ActorNearPlayer(a, NEAR_BOX))
            return 1;
        return spawn(a);
    case SPAWN_NEAR:
        set_spawn_wait(a, (uint16_t)(spawn_wait(a) - 1));
        if (!spawn_wait(a))
            return spawn(a);
        if (!ActorNearPlayer(a, NEAR_BOX))
            a->resume = SPAWN_COUNT;
        return 1;
    case DOWNED:
        AnimStep(a);
        if (--o->frames)
            return 1;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:A196 Kind28Second
 * WHAT IT DOES: Actor kind $28 in its second state (downed): sound $2C (and
 * $29 first when it never waited facing right; no script has that), pose 2
 * and 32 frames of its animation from the next frame, then its end.
 */
int Kind28Second(Actor *a)
{
    if (!own(a)->right)
        SoundQueueAdd(0x29);
    SoundQueueAdd(0x2C);
    AnimStart(a, 0x02);
    own(a)->frames = 0x20;
    a->resume = DOWNED;
    return 1;
}

/*
 * @routine 83:A1BD Kind28Wait
 * WHAT IT DOES: Kind $28's wait: its animation on; downed by shots; counts
 * the frames its facing has no bit 7 (+$46); once risen (+$45) stomped or
 * hurting as touched, else it bounces Aladdin and, when it does (no script
 * has it), is downed itself (state $8C, its wait routine off); ended when
 * out of range (box 8).
 */
int Kind28Wait(Actor *a)
{
    AnimStep(a);
    (void)ActorDownedByShot(a);
    if (!(a->facing & 0x80))
        own(a)->right++;
    if (own(a)->up) {
        (void)ActorStompOrHurt(a);
    } else if (ActorBouncesPlayer(a)) {
        a->flags &= (uint8_t)~0x80;
        a->state = 0x8C;
    }
    return ActorEndOutOfRange(a, 0x08);
}
