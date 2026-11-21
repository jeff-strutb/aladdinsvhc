/*
 * kind98.c: actor kind $98 (stage a5 s2), the flier that bobs along the
 * top of the screen, drops kind $9A, spawns kind $23, takes six hits and
 * throws kind $A5 after each; its wait is Kind98WaitWork.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "video/video.h"

#define TOP 0x38                /* the y (pixels' low byte) it bobs at */
#define RANDOM_LIST (0x3E >> 1) /* RandomFromList's list (the original's X = $3E): its way, 0 or 2 */
#define DROP_KIND 0x9A
#define SPAWN_KIND 0x23
#define THROW_KIND 0xA5
/* The facing its two spawns get: the original passes the X the actor loop
   leaves at a resume (its LDX #$08 at 83:94F1), which nothing between the
   frame's resume and the call changes. Their +$07 is the loop's Y, which
   kinds $9A and $23 do not read before they set it: 0 here. */
#define SPAWN_FACING 0x08

typedef struct {
    uint8_t b0a;
    uint8_t hits;               /* +$11: hits left (6 as it starts); counted down by the hit code */
    uint8_t before[OWN_40 - OWN_13];
    uint8_t frames;             /* +$40: frames left of a wait */
    uint8_t b41;
    uint8_t steps;              /* +$42: frames left of a move; throws left (the $07 of each kind $A5) */
    uint8_t b43;
    uint8_t left;               /* +$44: 0 it flies right across next, else left */
    uint8_t b45;
    uint8_t hit;                /* +$46: nonzero while hit: its wait does not hurt or turn shots */
    uint8_t b47;
    uint8_t turns;              /* +$48: nonzero: the next turn at the left edge spawns kind $23 */
    uint8_t rising;             /* +$49: ActorBobY's way */
} Kind98Own;

_Static_assert(sizeof(Kind98Own) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind98Own *own(Actor *a) { return (Kind98Own *)a->own.bytes; }

enum {
    START_SET = ACTOR_RESUME_OWN,   /* after its first $80 frames */
    START_POSE,                     /* after pose 0's $20 frames */
    CYCLE_BOB,                      /* each of the cycle's $60 frames of bobbing */
    ACROSS_1,                       /* way 0: its first $20 frames across */
    ACROSS_2,                       /* $10 frames in pose 3, then the drop */
    ACROSS_3,                       /* $20 frames more */
    FLIGHT,                         /* each frame across until the far side */
    SPAWN_BOB_1,                    /* the spawn: $10 frames of bobbing */
    SPAWN_BOB_2,                    /* $1A more after the spawn */
    END_BOB,                        /* the turn: $20 frames of bobbing */
    END_UP,                         /* each frame of its flight back up */
    HIT_POSE,                       /* after the hit pose's 8 frames */
    HIT_END,                        /* beaten: after its $30 frames */
    HIT_FALL,                       /* each frame of its fall to the ground */
    HIT_GROUND,                     /* after its $30 frames on the ground */
    HIT_UP,                         /* each frame of its rise */
    HIT_BOB,                        /* $10 frames of bobbing back at the top */
    THROW_BOB_1,                    /* a throw: $10 frames of bobbing first */
    THROW_BOB_2,                    /* $30 after it */
    STAY,
};

static uint8_t x_low(const Actor *a) { return (uint8_t)pixels(a->x); }
static uint8_t y_low(const Actor *a) { return (uint8_t)pixels(a->y); }
/* the pixels' low byte set, as the original's 8-bit stores */
static void set_x_low(Actor *a, uint8_t v) { a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0xFF00) | v)); }
static void set_y_low(Actor *a, uint8_t v) { a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | v)); }

/* `n` frames of the step at resume point `at`, from the next frame */
static int frames_of(Actor *a, uint8_t *count, uint8_t n, int at)
{
    *count = n;
    a->resume = (uint16_t)at;
    return 1;
}

/* Its speed across $0180 along its next way (84:A197 and 84:A35A). */
static void speed_across(Actor *a)
{
    a->facing = own(a)->left;
    a->speed_x = 0x180;                 /* 1.5 pixels a frame */
}

static int cycle(Actor *a);
static int cycle_clear_x(Actor *a);

/*
 * @routine 84:A25F Kind98StartEnd
 * WHAT IT DOES: Kind $98's turn: it bobs $20 frames, sounds $50, faces the
 * middle of the screen (left from its x + $60 at $80 on), flies back up to
 * its height (motion preset $70, pose 2) and goes the other way next; then
 * its cycle.
 */
static int start_end(Actor *a)
{
    return frames_of(a, &own(a)->steps, 0x20, END_BOB);
}

/*
 * @routine 84:A224 Kind98StartSpawn
 * WHAT IT DOES: Kind $98's second way: sound $47, pose 4, $10 frames of
 * bobbing, a kind $23, $1A frames more, pose 2, then its turn.
 */
static int start_spawn(Actor *a)
{
    own(a)->turns = 0;
    SoundQueueAdd(0x47);
    AnimStart(a, 0x04);
    return frames_of(a, &own(a)->frames, 0x10, SPAWN_BOB_1);
}

/*
 * @routine 84:A167 Kind98Cycle
 * WHAT IT DOES: Kind $98's cycle: back at its height (fraction 0), its bob
 * going down, no speed down, gravity 6; then Kind98CycleClearX.
 */
static int cycle(Actor *a)
{
    set_y_low(a, TOP);
    a->y &= ~0xFFu;
    own(a)->rising = 0;
    ActorSpeedYClear(a);
    a->accel_y = 0x06;
    return cycle_clear_x(a);
}

/*
 * @routine 84:A177 Kind98CycleClearX
 * WHAT IT DOES: No speed across; $60 frames of bobbing; then by a random
 * pick (list $3E) it flies across (way 0: $20 frames, pose 3 and sound
 * $2F, $10 frames, a kind $9A dropped, $20 frames, pose 2, its flight) or
 * spawns a kind $23 (way 2, Kind98StartSpawn).
 * Dead, not converted: @routine 84:A20F Kind98FlightDead, a JMP to here
 * right after Kind98Flight's JMP to Kind98StartEnd; nothing in the ROM
 * names $84:A20F and no input reaches it (the record's own proof; its
 * coverage came from a record whose resume address was set to it).
 */
static int cycle_clear_x(Actor *a)
{
    ActorSpeedXClear(a);
    return frames_of(a, &own(a)->frames, 0x60, CYCLE_BOB);
}

/* A move across for `n` frames, at `at`. */
static int across(Actor *a, uint8_t n, int at)
{
    return frames_of(a, &own(a)->steps, n, at);
}

/* each frame across: moved, bobbed; returns nonzero when its count ran out */
static int across_step(Actor *a)
{
    ActorMoveX(a);
    ActorBobY(a);
    return --own(a)->steps == 0;
}

/*
 * @routine 84:A389 Kind98Throw
 * WHAT IT DOES: Kind $98 throws a kind $A5, its +$07 the throws left, with
 * sound $3E.
 */
static void throw(Actor *a)
{
    SoundQueueAdd(0x3E);
    a->spawn_byte = own(a)->steps;
    a->spawn_at = 0;
    (void)ActorCreateChild8(a, THROW_KIND);
}

/* A throw: pose 7 and $10 frames of bobbing first. */
static int throw_round(Actor *a)
{
    AnimStart(a, 0x07);
    return frames_of(a, &own(a)->frames, 0x10, THROW_BOB_1);
}

/* one frame of bobbing; returns nonzero when the count ran out */
static int bob_step(Actor *a, uint8_t *count)
{
    ActorBobY(a);
    return --*count == 0;
}

/*
 * @routine 84:A112 Kind98Start
 * WHAT IT DOES: Actor kind $98: holds the screen still (both scroll holds'
 * low bytes 1), six hits, waits $80 frames; then +$08 bits 0-1 and its
 * wait routine on, its probe list and picture list, x $80 and y $38 (the
 * pixels' low bytes), right next, sound $46, pose 0 for $20 frames, pose 2;
 * then over and over (Kind98Cycle) bobs at the top and flies across or
 * spawns a kind $23, turns, flies back up and goes the other way.
 * Its second entry is Kind98Hit.
 */
int Kind98Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        own(a)->hits = 6;
        g.boss_beaten = 0;
        own(a)->turns = 0;
        g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 0x01);
        g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
        a->resume = START_SET;
        return 0x80;
    case START_SET:
        a->anim_flags |= 0x03;
        a->flags |= 0x90;
        a->probes = k98_probes();
        a->own_pictures = k98_pictures();
        set_x_low(a, 0x80);
        set_y_low(a, TOP);
        own(a)->left = 1;
        own(a)->hit = 0;
        a->facing = 0;
        SoundQueueAdd(0x46);
        AnimStart(a, 0x00);
        a->resume = START_POSE;
        return 0x20;
    case START_POSE:
        AnimStart(a, 0x02);
        return cycle(a);
    case CYCLE_BOB:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        switch ((uint8_t)RandomFromList(RANDOM_LIST)) {
        case 0:
            speed_across(a);
            return across(a, 0x20, ACROSS_1);
        case 2:
            return start_spawn(a);
        default: {
            static int told;

            if (!told++)
                fprintf(stderr, "Kind98CycleClearX: a way past its table not converted\n");
            a->resume = STAY;
            return 1;
        }
        }
    case ACROSS_1:
        if (!across_step(a))
            return 1;
        AnimStart(a, 0x03);
        SoundQueueAdd(0x2F);
        return across(a, 0x10, ACROSS_2);
    case ACROSS_2:
        if (!across_step(a))
            return 1;
        (void)ActorSpawnFacing(a, DROP_KIND, SPAWN_FACING, 0);
        return across(a, 0x20, ACROSS_3);
    case ACROSS_3:
        if (!across_step(a))
            return 1;
        AnimStart(a, 0x02);
        a->resume = FLIGHT;
        return 1;
    case FLIGHT:
        /* @routine 84:A1F3 Kind98Flight: each frame it moves and bobs
           until it reaches the far side: flying left, x + $60 below $20
           (it goes right next) and it turns; flying right, x + $60 at $E0
           or more (left next), and it spawns a kind $23 first when it last
           turned after a hit */
        ActorMoveX(a);
        ActorBobY(a);
        if (own(a)->left) {
            if ((uint8_t)(x_low(a) + 0x60) >= 0x20)
                return 1;
            own(a)->left = 0;
            return start_end(a);
        }
        if ((uint8_t)(x_low(a) + 0x60) < 0xE0)
            return 1;
        own(a)->left++;
        if (own(a)->turns)
            return start_spawn(a);
        return start_end(a);
    case SPAWN_BOB_1:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        (void)ActorSpawnFacing(a, SPAWN_KIND, SPAWN_FACING, 0);
        return frames_of(a, &own(a)->frames, 0x1A, SPAWN_BOB_2);
    case SPAWN_BOB_2:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        AnimStart(a, 0x02);
        return start_end(a);
    case END_BOB:
        if (!bob_step(a, &own(a)->steps))
            return 1;
        SoundQueueAdd(0x50);
        a->facing = (uint8_t)(x_low(a) + 0x60) >= 0x80 ? 1 : 0;
        ActorMotionPreset(a, 0x70);
        AnimStart(a, 0x02);
        a->resume = END_UP;
        return 1;
    case END_UP:
        ActorFlyAgainst(a);
        if (y_low(a) > TOP)
            return 1;
        set_y_low(a, TOP);
        a->y &= ~0xFFu;
        own(a)->left ^= 0x01;
        return cycle(a);
    case ACTOR_START_SECOND:
        return Kind98Hit(a);
    case HIT_POSE:
        if (!own(a)->hits) {
            g.scene_cue_first = 1;
            g.boss_beaten++;
            AnimStart(a, 0x01);
            SoundQueueAdd(0x46);
            a->resume = HIT_END;
            return 0x30;
        }
        ActorSpeedXClear(a);
        ActorSpeedYSetGravity(a, TOP, 0x8D);
        a->resume = HIT_FALL;
        return 1;
    case HIT_END:
        return ActorEndPlaced(a);
    case HIT_FALL:
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 0x06);
        a->resume = HIT_GROUND;
        return 0x30;
    case HIT_GROUND:
        a->anim_flags |= 0x10;
        AnimStart(a, 0x02);
        ActorSpeedYSet(a, 0x90);
        a->resume = HIT_UP;
        return 1;
    case HIT_UP:
        ActorMoveY(a);
        if (y_low(a) > TOP)
            return 1;
        set_y_low(a, TOP);
        own(a)->rising = 0;
        ActorSpeedYClear(a);
        a->accel_y = 0x06;
        a->anim_flags &= (uint8_t)~0x10;
        own(a)->hit = 0;
        return frames_of(a, &own(a)->frames, 0x10, HIT_BOB);
    case HIT_BOB:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        own(a)->steps = 3;
        return throw_round(a);
    case THROW_BOB_1:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        throw(a);
        return frames_of(a, &own(a)->frames, 0x30, THROW_BOB_2);
    case THROW_BOB_2:
        if (!bob_step(a, &own(a)->frames))
            return 1;
        if (--own(a)->steps)
            return throw_round(a);
        AnimStart(a, 0x02);
        /* the original sets its state byte to $0C here; the yield that
           follows marks it waiting, as the loop does */
        speed_across(a);
        own(a)->turns++;
        a->resume = FLIGHT;
        return 1;
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 84:A2A6 Kind98Hit
 * WHAT IT DOES: Kind $98 hit (its second entry): sound $0F and its hit
 * pose for 8 frames, its wait routine on. With no hits left it is beaten:
 * g.scene_cue_first set, g.boss_beaten up, its last pose with sound $46,
 * $30 frames and it ends. Else it falls to the ground (gravity $38, speed
 * $8D), shows its hurt pose $30 frames, rises back to the top (+$08 bit 4
 * while it does), bobs $10 frames, throws three kind $A5 ($10 and $30
 * frames about each) and flies across again, spawning kind $23 at the next
 * turn.
 */
int Kind98Hit(Actor *a)
{
    own(a)->hit++;
    a->flags |= 0x80;
    SoundQueueAdd(0x0F);
    AnimStart(a, 0x05);
    a->resume = HIT_POSE;
    return 8;
}

/*
 * @routine 84:A39C Kind98WaitWork
 * WHAT IT DOES: What kind $98 does each frame it waits: its animation
 * steps and its picture is sent; it turns to Aladdin (its flip); unless
 * hit, Aladdin coming down on it or touching it, and his shots turning on
 * it, are dealt with. Kind $98's wait routine (through its stub 83:93C2).
 */
int Kind98WaitWork(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    a->flip = ActorSideOfPlayer(a);
    if (!own(a)->hit) {
        (void)ActorStompOrHurt(a);
        (void)ActorTurnsShotBack(a);
    }
    return ACTOR_GOES_ON;
}
