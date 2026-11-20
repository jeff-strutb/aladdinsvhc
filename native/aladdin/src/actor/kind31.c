/*
 * kind31.c: actor kind $31: one of eight ways of waiting for Aladdin, then
 * its walk (turning at edges, hopping up walls) or its leap at him; its
 * second state, which drops it away; the tiles it breaks; its wait.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "map/map.h"
#include "video/video.h"

#define SPAWN_LOOK 0x44         /* the look its spawn takes (ActorSpawnLookWait's +$4D) */
#define GOTO_SET 0x0A           /* the direction set it flies to a point with */
#define TILE_LEFT 0x7FB8        /* the map tiles it leaves above it, left and right */
#define TILE_RIGHT 0x7FC0

/* its own bytes */
static uint8_t *own8(Actor *a, int at) { return &a->own.bytes[at]; }
static uint16_t own16(Actor *a, int at) { return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8); }
static void set16(Actor *a, int at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

#define FRAMES OWN_40           /* frames left of a wait or a step */
#define MET OWN_42              /* 0 waiting; 1 met Aladdin (its wait turns it and it can be downed); $80 leaping */
#define OFFSET OWN_45           /* (word) the way's offset added to Aladdin's x */
#define HOME_X OWN_44           /* (word) where it leapt from; then +$45 its aim back */
#define HOME_Y OWN_46           /* (word) */
#define AIM OWN_45
/* ActorGoToPoint's point and set, ActorSpawnLookWait's look: +$48, +$4A, +$4C, +$4D */
#define GOTO_X OWN_48
#define GOTO_Y OWN_4A
#define GOTO_SPEED OWN_4C
#define LOOK OWN_4D

enum {
    W0_POSE = ACTOR_RESUME_OWN, /* way 0: a frame in, then the rise */
    W0_RISE,                    /* ActorRiseWhenInReach, box 0 */
    MEET,                       /* after a pose 2's 8 frames: pose 0, it faces him, on to its walk */
    LEFT,                       /* each frame until he is $41-$60 left of it */
    RIGHT,                      /* each frame until he is $21-$40 right of it */
    COUNT,                      /* each frame of +$40's count, then the spawn and the hop */
    SPAWN,                      /* ActorSpawnLookWait */
    HOP_UP,                     /* ActorHopFromTable $14, after the spawn */
    NEAR,                       /* each frame until he is within $20 of its x less +$45 */
    AHEAD,                      /* each frame until he is within its next $20 (way 4) */
    AHEAD_ONE,                  /* the same, then a frame (way 5) */
    RANDOM_COUNT,               /* each frame of the random count, then the tiles and the hop */
    HOP_BREAK,                  /* ActorHopFromTable $14, after the tiles */
    DROP,                       /* each frame of its fall for good */
    WALK,                       /* each frame of the walk */
    WALL_POSE,                  /* after pose 1's 16 frames at a wall */
    WALL_HOP,                   /* ActorHopFromTable $0F */
    WALL_LAND,                  /* after pose 2's 8 frames */
    LEAP,                       /* each frame of the leap up */
    LEAP_TOP,                   /* after its 8 frames at the top */
    TO_HIM,                     /* ActorGoToPoint: to where he was */
    BACK,                       /* each frame of its $30 along its aim back */
    TO_HOME,                    /* ActorGoToPoint: back where it leapt from */
    HOME_RISE,                  /* ActorRiseWhenInReach, box 0 */
    STAY,
    SECOND_STEP,                /* the second state: each of its 12 frames */
};

static uint16_t player_x(void) { return pixels(g.actors[0].x); }
static uint16_t player_y(void) { return pixels(g.actors[0].y); }

/* A shared routine that waits inside it, called at resume point `at`:
   `r` its answer. Returns nonzero when it is back to its caller. */
static int returned(int r) { return r == ACTOR_RETURNS || r == ACTOR_RETURNS_C; }

static int walk(Actor *a);
static int walk_way(Actor *a);

/*
 * @routine 83:FD6D ActorProbeAheadInGround
 * WHAT IT DOES: Tests two points for ground: `ahead` pixels in front of the
 * actor (by its facing) and `up` above it, then `up` above its own x: true
 * when either is in the ground (its tile's behaviour has any of bits
 * 0-2). Kind $31 is its only caller (point_moves.c's record; its piece).
 */
static bool probe_ahead_in_ground(const Actor *a, uint16_t ahead, uint16_t up)
{
    MapPoint p;

    p.x = (uint16_t)(pixels(a->x) + (a->facing ? (uint16_t)-ahead : ahead));
    p.y = (uint16_t)(pixels(a->y) - up);
    (void)PointInGround(a, &p);
    if (p.behaviour & 0x07)
        return true;
    p.x = pixels(a->x);
    (void)PointInGround(a, &p);
    return (p.behaviour & 0x07) != 0;
}

/*
 * @routine 83:FC5D ActorGoToPoint
 * WHAT IT DOES: Moves the actor toward the point at its +$48, +$4A
 * (pixels) a step a frame by direction set +$4C, facing the way it goes,
 * until it is within box $20 of it; then back to its caller. A routine
 * that waits inside (actor.h): called again at its caller's resume point
 * each frame until it returns ACTOR_RETURNS; Actor.call_at 1 once it has
 * stepped. Kind $31 is its only caller (point_moves.c's record; its piece).
 */
static int go_to_point(Actor *a)
{
    uint16_t x = own16(a, OWN_48), y = own16(a, OWN_4A);

    if (a->call_at && ActorNearPoint(a, 0x20, x, y)) {
        a->call_at = 0;
        return ACTOR_RETURNS;
    }
    a->facing = ActorAimAt(a, x, y);
    ActorMoveByVector(a, *own8(a, OWN_4C));
    a->call_at = 1;
    return 1;
}

/* pose 2 for 8 frames, then MEET */
static int land(Actor *a)
{
    AnimStart(a, 0x02);
    a->resume = MEET;
    return 8;
}

/* `n` frames counted from the next, at resume point `at` */
static int count(Actor *a, uint8_t n, int at)
{
    *own8(a, FRAMES) = n;
    a->resume = (uint16_t)at;
    return 1;
}

/* After its count (83:AC8E): a sound if it can be hit, pose 6, a spawn of
   look $44 beside it, then a hop. */
static int spawn_and_hop(Actor *a)
{
    if (a->flags & 0x40)
        SoundQueueAdd(0x1D);
    AnimStart(a, 0x06);
    a->anim_flags |= 0x08;
    a->spawn_at = 0;
    *own8(a, LOOK) = SPAWN_LOOK;
    a->resume = SPAWN;
    return Kind31Start(a);
}

/* A random wait (83:AD09): the first entry of random list 0's frames. */
static int random_wait(Actor *a)
{
    return count(a, (uint8_t)RandomFromList(0), RANDOM_COUNT);
}

/* Is (base + add - sub) within 0-$1F (16-bit)? */
static int within_20(uint16_t base, uint16_t add, uint16_t sub)
{
    return (uint16_t)(base + add - sub) < 0x20;
}

/*
 * @routine 83:AD90 Kind31Drop
 * WHAT IT DOES: Kind $31's way 7 (and its second state's end): +$42 0,
 * pose 3, speeds 6 across and $0F down, gravity $28; each frame from the
 * next it moves across and falls, for good.
 */
static int drop(Actor *a)
{
    *own8(a, MET) = 0;
    AnimStart(a, 0x03);
    a->anim_flags |= 0x08;
    ActorSpeedXSet(a, 0x06);
    ActorSpeedYSet(a, 0x0F);
    a->accel_y = 0x28;
    a->resume = DROP;
    return 1;
}

static void turn(Actor *a)
{
    a->facing ^= 0x01;
    a->flip = a->facing;
}

/* the walk: its speed, then a frame */
static int walk(Actor *a)
{
    ActorSpeedXSet(a, 0x15);
    a->resume = WALK;
    return 1;
}

/* Its walk by +$07 (83:ADC4): 0 walks; 2 and 4 every 32 frames look for
   Aladdin within $60 across and $40 up or down and leap at him. */
static int walk_way(Actor *a)
{
    switch (a->spawn_byte) {
    case 0:
        return walk(a);
    case 2:
    case 4:
        if (--*own8(a, FRAMES))
            return walk(a);
        *own8(a, FRAMES) = 0x20;
        if ((uint16_t)(pixels(a->x) - player_x() + 0x60) >= 0xC0
            || (uint16_t)(pixels(a->y) - player_y() + 0x40) >= 0x80)
            return walk(a);
        AnimStart(a, 0x01);
        ActorSpeedYSetGravity(a, 0x38, 0x03);
        a->resume = LEAP;
        return 1;
    default: {
        static int told;

        /* past the table of walks: the original jumps where the next
           words point */
        if (!told++)
            fprintf(stderr, "Kind31Walk: walk %u (past its table) not converted\n", a->spawn_byte);
        a->resume = STAY;
        return 1;
    }
    }
}

/*
 * @routine 83:ADB8 Kind31Walk
 * WHAT IT DOES: Kind $31's walk once it has met Aladdin: its +$07 becomes
 * bits 3-4 of it over 4; then by it it walks (speed $15), turning off an
 * edge (no ground within 3 pixels) and, at a wall, turning if the wall goes
 * on above ($10 ahead, $38 up), else hopping up it (pose 1 for 16 frames,
 * hop $0F, pose 2 for 8); or (2, 4) every 32 frames, with Aladdin within
 * $60 across and $40 up or down, it leaps (pose 1, gravity $38, speed 3)
 * until it falls, sounds $2F, poses 4 for 8 frames (+$42 $80), flies to
 * where he was, back along its aim for $30 frames, back home, rises when
 * in reach, and walks on (+$42 1, +$07 0).
 */
static int walk_start(Actor *a)
{
    a->spawn_byte = (uint8_t)((a->spawn_byte & 0x18) >> 2);
    *own8(a, FRAMES) = 1;
    return walk_way(a);
}

/* pose 0, +$42 counted up, facing him, then its walk */
static int meet(Actor *a)
{
    AnimStart(a, 0x00);
    ++*own8(a, MET);
    ActorFacePlayer(a);
    return walk_start(a);
}

/*
 * @routine 83:AF57 Kind31BreakTiles
 * WHAT IT DOES: Changes the two map tiles above the actor (8 pixels up,
 * 8 left and 8 right) and marks its placement never placed again.
 */
static void break_tiles(Actor *a)
{
    uint16_t x = pixels(a->x), y = pixels(a->y);

    TileChangeAdd(TILE_LEFT, (uint16_t)(x - 8), (uint16_t)(y - 8));
    TileChangeAdd(TILE_RIGHT, (uint16_t)(x + 8), (uint16_t)(y - 8));
    g.placed[a->placement] = 0x80;
}

/*
 * @routine 83:ABEA Kind31Start
 * WHAT IT DOES: Actor kind $31: by its +$07 (bits 0-2) one of eight ways of
 * waiting for Aladdin, then on into its walk (Kind31Walk); from progress
 * sequence 2 on its palette from the ROM ($3F) and another table of ways.
 * The ways, before sequence 2: 0 rises when he is in reach; 1-3 wait until
 * he is $41-$60 left of it, then 1, $20 or $40 frames; 4-6 until he is
 * $21-$40 right of it; then each spawns look $44 beside it and hops. From
 * sequence 2: 0 the same; 1, 2, 3, 6 wait until he is within $20 of its x
 * less $70, $30, 0 or $C0; 4 until he is within its next $20; 5 the same
 * and a frame; then a random wait, its tiles broken, sound $2E, pose 3 and
 * a hop. Way 7 drops (Kind31Drop). After a hop: pose 2 for 8 frames, pose
 * 0, +$42 up, facing him, its walk. Its start sets +$08 bit 1 (bit 3
 * clear), its wait routine on ($09 $90), its probe list, +$42 and its
 * facing 0. Its second entry is Kind31Second.
 */
int Kind31Start(Actor *a)
{
    int r;

    switch (a->resume) {
    case ACTOR_START: {
        uint8_t way = (uint8_t)(a->spawn_byte & 0x07);

        a->anim_flags = (uint8_t)((a->anim_flags | 0x02) & ~0x08);
        a->flags |= 0x90;
        a->probes = k31_probes();
        *own8(a, MET) = 0;
        a->facing = 0;
        a->flip = 0;
        if (g.progress_sequence >= 2)
            ActorPaletteFromRom(a, 0x3F);
        if (way == 7)
            return drop(a);
        if (way == 0) {
            a->resume = W0_POSE;
            return 1;
        }
        if (g.progress_sequence < 2) {
            if (way <= 3)
                return count(a, way == 1 ? 0x01 : way == 2 ? 0x20 : 0x40, LEFT);
            a->resume = RIGHT;
            return 1;
        }
        if (way == 4) {
            a->resume = AHEAD;
            return 1;
        }
        if (way == 5) {
            a->resume = AHEAD_ONE;
            return 1;
        }
        set16(a, OFFSET, way == 1 ? 0x70 : way == 2 ? 0x30 : way == 3 ? 0x00 : 0xC0);
        a->resume = NEAR;
        return 1;
    }
    case ACTOR_START_SECOND:
        return Kind31Second(a);
    case W0_POSE:
        AnimStart(a, 0x00);
        a->anim_flags |= 0x08;
        a->resume = W0_RISE;
        return Kind31Start(a);
    case W0_RISE:
        r = ActorRiseWhenInReach(a, 0x00);
        if (!returned(r))
            return r;
        return land(a);
    case MEET:
        return meet(a);
    case LEFT:
        if (!within_20(player_x(), 0x60, pixels(a->x)))
            return 1;
        a->resume = COUNT;
        return 1;
    case RIGHT:
        if (!within_20(pixels(a->x), 0x40, player_x()))
            return 1;
        return spawn_and_hop(a);
    case COUNT:
        if (--*own8(a, FRAMES))
            return 1;
        return spawn_and_hop(a);
    case SPAWN:
        r = ActorSpawnLookWait(a);
        if (!returned(r))
            return r;
        a->resume = HOP_UP;
        return Kind31Start(a);
    case HOP_UP:
        r = ActorHopFromTable(a, 0x14);
        if (!returned(r))
            return r;
        return land(a);
    case NEAR:
        if (!within_20(own16(a, OFFSET), player_x(), pixels(a->x)))
            return 1;
        return random_wait(a);
    case AHEAD:
        if (within_20(pixels(a->x), 0x20, player_x()))
            return random_wait(a);
        return 1;
    case AHEAD_ONE:
        if (!within_20(pixels(a->x), 0x20, player_x()))
            return 1;
        return count(a, 0x01, RANDOM_COUNT);
    case RANDOM_COUNT:
        if (--*own8(a, FRAMES))
            return 1;
        break_tiles(a);
        SoundQueueAdd(0x2E);
        AnimStart(a, 0x03);
        a->anim_flags |= 0x08;
        a->resume = HOP_BREAK;
        return Kind31Start(a);
    case HOP_BREAK:
        r = ActorHopFromTable(a, 0x14);
        if (!returned(r))
            return r;
        return land(a);
    case DROP:
        ActorMoveX(a);
        ActorFall(a);
        return 1;
    case WALK:
        ActorMoveX(a);
        if (ActorOutOfWallProbe4(a)) {
            if (probe_ahead_in_ground(a, 0x10, 0x38)) {
                turn(a);
                return 1;
            }
            AnimStart(a, 0x01);
            a->resume = WALL_POSE;
            return 0x10;
        }
        {
            MapPoint p;

            if (ActorSnapToGround(a, &p))
                return walk_way(a);
        }
        turn(a);
        return 1;
    case WALL_POSE:
        a->resume = WALL_HOP;
        return Kind31Start(a);
    case WALL_HOP:
        r = ActorHopFromTable(a, 0x0F);
        if (!returned(r))
            return r;
        AnimStart(a, 0x02);
        a->resume = WALL_LAND;
        return 8;
    case WALL_LAND:
        AnimStart(a, 0x00);
        return walk(a);
    case LEAP:
        ActorFall(a);
        if (a->speed_y < 0)
            return 1;
        SoundQueueAdd(0x2F);
        AnimStart(a, 0x04);
        *own8(a, MET) = 0x80;
        a->resume = LEAP_TOP;
        return 8;
    case LEAP_TOP:
        set16(a, HOME_X, pixels(a->x));
        set16(a, HOME_Y, pixels(a->y));
        set16(a, GOTO_X, player_x());
        set16(a, GOTO_Y, player_y());
        *own8(a, GOTO_SPEED) = GOTO_SET;
        a->resume = TO_HIM;
        return Kind31Start(a);
    case TO_HIM: {
        uint8_t aim;

        r = go_to_point(a);
        if (!returned(r))
            return r;
        set16(a, GOTO_X, own16(a, HOME_X));
        set16(a, GOTO_Y, own16(a, HOME_Y));
        aim = ActorAimAt(a, own16(a, HOME_X), own16(a, HOME_Y));
        *own8(a, AIM) = aim;
        a->facing = (uint8_t)(aim ^ 0x10);
        ActorSpeedSetVector(a, GOTO_SET);
        a->facing = aim;
        return count(a, 0x30, BACK);
    }
    case BACK:
        ActorSpeedAddVector(a, 0x0C);
        ActorMoveRight(a);
        ActorMoveY(a);
        if (--*own8(a, FRAMES))
            return 1;
        a->resume = TO_HOME;
        return Kind31Start(a);
    case TO_HOME:
        r = go_to_point(a);
        if (!returned(r))
            return r;
        a->resume = HOME_RISE;
        return Kind31Start(a);
    case HOME_RISE:
        r = ActorRiseWhenInReach(a, 0x00);
        if (!returned(r))
            return r;
        AnimStart(a, 0x00);
        *own8(a, MET) = 1;
        a->facing = a->flip;
        a->spawn_byte = 0;
        return walk_way(a);
    case SECOND_STEP:
        AnimStep(a);
        if (--*own8(a, FRAMES))
            return 1;
        a->spawn_byte = 0x07;
        a->spawn_at = 0;
        (void)ActorSpawnSameLook(a, a->kind);
        a->facing = 1;
        a->flip = 1;
        a->flags |= 0x80;
        return drop(a);
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:AF1C Kind31Second
 * WHAT IT DOES: Kind $31's second entry (state $80): sound $0F; its state
 * back to $0C and +$11 0; pose 2 and 12 frames of its animation from the
 * next frame; then a copy of its own kind and look beside it (+$07 7: it
 * drops), facing right, its wait routine on, and it drops (Kind31Drop).
 * Its state byte's $0C is overwritten by the loop at its first wait, as the
 * original's BRK does.
 */
int Kind31Second(Actor *a)
{
    SoundQueueAdd(0x0F);
    a->own.bytes[OWN_11] = 0;
    AnimStart(a, 0x02);
    return count(a, 0x0C, SECOND_STEP);
}

/*
 * @routine 83:AEEC Kind31Wait
 * WHAT IT DOES: Kind $31's wait: once it has met Aladdin (+$42 set), on
 * levels below 7 it turns round at x $0F00 and beyond, and a shot downs it
 * or else it stomps or hurts him; then its animation on, and it is ended
 * once out of range (box $38).
 */
int Kind31Wait(Actor *a)
{
    if (*own8(a, MET)) {
        if (g.level < 7 && pixels(a->x) >= 0x0F00)
            turn(a);
        if (!ActorDownedByShot(a))
            (void)ActorStompOrHurt(a);
    }
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x38);
}
