/*
 * kind6d.c: actor kind $6D (the pots and the like): its start, eight ways
 * of moving by its +$07, what it does once downed (its second entry), and
 * its wait routine.
 */
#include "actor/actor_own.h"
#include "video/view.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"
#include "map/map.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a leg, a turn or a wait */
    uint8_t turns;              /* +$41: the homing way's turns left; then its turn step (1 or $FF) */
    uint8_t between[OWN_47 - OWN_42];
    uint8_t step;               /* +$47: the spawner's own +$07, kept while a child is made */
    uint8_t downed;             /* +$48: counted up by its wait as a shot downs it */
    uint8_t frame_seen;         /* +$49: the up-and-down way clears it; ActorYByFrame keeps the frame here */
} K6DOwn;

static inline K6DOwn *k6d(Actor *a) { return (K6DOwn *)a->own.bytes; }

enum {
    ACROSS_SPEED = ACTOR_RESUME_OWN,    /* way 0: speeding up across */
    ACROSS_SLOW,                /* slowing to a stop */
    ACROSS_LEG,                 /* after the turn's $18 frames: the next leg */
    UPDOWN_SPEED,               /* way 1: speeding up or down */
    UPDOWN_SLOW,                /* slowing to a stop */
    RUN,                        /* way 2: the run */
    RUN_RISE,                   /* rising, gravity $30 */
    RUN_SLOW,                   /* the rise slowing */
    AWAY,                       /* across for good (Kind6DAway) */
    HOP_AIR,                    /* ways 3 and 7: in the air */
    HOP_LAND,                   /* after pose 2's 8 frames */
    HOP_AGAIN,                  /* after pose 1's 8 frames */
    HOME,                       /* ways 4 and 6: homing, 12 frames a turn */
    HOME_UP,                    /* turning on until it points up, for good */
    SPAWN_WAIT,                 /* way 5: $E0 frames after a child */
    SPAWN_FAR,                  /* until $90 or more from the range point */
    DOWN_HOLD,                  /* downed: 10 frames */
    DOWN_FLY                    /* downed: flying until out of range */
};

#define SPEED_TOP 0x00C0u       /* the speed (its low two bytes) at which ways 0 and 1 stop speeding up */
#define FALL_CUT 0x0100u        /* way 7: falling this fast or more, its gravity cut */
#define SPAWN_NEAR (uint16_t)(0x0090 + view.dx)      /* way 5: within this of the range point, no child */

/* ActorDecelerateX and Y leave a speed that would go below 0 at 0; the
   original's ways 0 and 1 end a leg on the N their subtraction leaves:
   these say, before the call, that it will. */
static int slows_past_zero_x(const Actor *a)
{
    return signed24((uint32_t)a->speed_x - a->accel_x) < 0;
}

static int slows_past_zero_y(const Actor *a)
{
    return signed24((uint32_t)a->speed_y - a->accel_y) < 0;
}

/* way 1's vertical step: up (ActorStepYBack) while facing is nonzero, else down */
static void step_y(Actor *a)
{
    if (a->facing)
        ActorStepYBack(a);
    else
        ActorStepY(a);
}

/* way 0: a leg's start */
static int across_leg(Actor *a)
{
    a->accel_x = 0x04;
    k6d(a)->frames = 0x60;
    a->resume = ACROSS_SPEED;
    return 1;
}

/* way 1: a leg's start */
static int updown_leg(Actor *a)
{
    ActorMotionReset(a, 0x03);
    k6d(a)->frames = 0x60;
    a->resume = UPDOWN_SPEED;
    return 1;
}

/* ways 3 and 7: the motion preset by way (3: $55, 7: $7F; +$07 bit 4 tells them apart) */
static uint8_t hop_preset(const Actor *a)
{
    return (a->spawn_byte & 0x10) ? 0x7F : 0x55;
}

/* ways 4 and 6, the up phase: unless it points up ($0C), every 12 frames
   one turn on by its step */
static void home_up_turn(Actor *a)
{
    K6DOwn *k = k6d(a);

    if ((a->facing & 0x0F) != 0x0C && !--k->frames) {
        a->facing = (uint8_t)((a->facing + k->turns) & 0x0F);
        k->frames = 0x0C;
    }
}

/* way 5: a try at a child of its own kind, its +$07 4 more while it is
   made (kept so when none is free) */
static int spawn_try(Actor *a)
{
    K6DOwn *k = k6d(a);

    k->frames = 0xE0;
    a->spawn_byte = (uint8_t)(a->spawn_byte + 4);
    a->spawn_at = 0;
    if (ActorSpawnSameLook(a, a->kind)) {
        a->spawn_byte = k->step;
        a->resume = SPAWN_WAIT;
        return 1;
    }
    a->resume = SPAWN_FAR;
    return 1;
}

/* the distance across from the point actors are kept in range of */
static uint16_t from_range_x(const Actor *a)
{
    uint16_t d = (uint16_t)(g.range_x - pixels(a->x));

    return (d & 0x8000) ? (uint16_t)-d : d;
}

/*
 * @routine 83:ECD4 Kind6DStart
 * WHAT IT DOES: Actor kind $6D's start: anim_flags 2, flags $90 (its wait
 * routine on), its probe list, stopped both ways; facing by +$07 bits 0-1
 * (1: right, 2 or 3: left) or, with them 0, toward the player
 * (ActorSideOfPlayer); +$48 0. Then by bits 2-4 (the table at $83:ED23):
 * 0 back and forth across: pose 8, no gravity; each leg $60 frames of
 * moving and speeding up by 4 (no more at speed $C0), its flip turned,
 * slowing to a stop, its facing turned, $18 frames still.
 * 1 up and down: pose 8, facing 0, +$49 0; each leg ActorMotionReset 3,
 * $60 frames of stepping (up while facing is nonzero) and speeding up (no
 * more at $C0), gravity 3, slowing to a stop, facing turned.
 * 2 a run that rises away: poses 2 then 4, acceleration 2, speed entry
 * $5A, $40 frames of moving and speeding up; pose 8, no acceleration,
 * gravity $30, $10 frames rising and moving; rising, moving and slowing
 * while its vertical speed is not negative; then across for good.
 * 3 and 7: hops by preset $55 (3) or $7F (7), from no vertical speed:
 * pose 8 and flying until out of the ground (3 turns round at a wall; 7
 * cuts its gravity while falling at $0100 or more), pose 2 for 8 frames,
 * pose 1 for 8, and again.
 * 4 and 6: a homing flight: pose 8, its facing times 8 as a direction,
 * 20 legs of 12 frames by vector set $18, each after the first turning
 * one step toward ActorDirectionToPlayer; then each 12 frames one step on
 * the way its flip says, until it points up, by vector set $18 for good.
 * 5 a spawner: a child of its own kind (+$07 4 more, spawn point 0,
 * ActorSpawnSameLook); made, $E0 frames; then each frame until $90 or more
 * from the range point, and again.
 * The original first counts a byte of copier RAM ($70:3000 + X) up and,
 * when the count stays written, fills its record's $01-$40 with $AA and
 * clears its kind; on the cartridge that byte is open bus ($70, the
 * operand's bank byte) and never reads back what was written, so it is
 * not converted.
 * Its second entry (83:EFF0 Kind6DSecond, downed) is below in the same
 * function.
 */
int Kind6DStart(Actor *a)
{
    K6DOwn *k = k6d(a);
    uint8_t dir, diff;
    int stops;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x02;
        a->flags = 0x90;
        a->probes = k6d_probes();
        ActorStopX(a);
        ActorStopY(a);
        a->facing = (a->spawn_byte & 0x03) ? (uint8_t)((a->spawn_byte & 0x03) >> 1) : ActorSideOfPlayer(a);
        a->flip = a->facing;
        k->downed = 0;
        switch ((a->spawn_byte & 0x1C) >> 2) {
        case 0:
            AnimStart(a, 8);
            ActorMotionReset(a, 0);
            return across_leg(a);
        case 1:
            AnimStart(a, 8);
            a->facing = 0;
            k->frame_seen = 0;
            return updown_leg(a);
        case 2:
            AnimStart(a, 2);
            AnimStart(a, 4);
            a->accel_x = 0x02;
            ActorSpeedXSet(a, 0x5A);
            k->frames = 0x40;
            a->resume = RUN;
            return 1;
        case 4:
        case 6:
            AnimStart(a, 8);
            k->turns = 0x14;
            a->facing = (uint8_t)(a->facing << 3);
            k->frames = 0x0C;
            a->resume = HOME;
            return 1;
        case 5:
            k->step = a->spawn_byte;
            return spawn_try(a);
        default:                        /* 3 and 7 */
            ActorMotionPreset(a, hop_preset(a));
            a->speed_y = 0;
            AnimStart(a, 8);
            a->resume = HOP_AIR;
            return 1;
        }

    case ACROSS_SPEED:
        ActorMoveX(a);
        ActorAccelerateX(a);
        if ((uint16_t)a->speed_x >= SPEED_TOP)
            a->accel_x = 0;
        if (--k->frames)
            return 1;
        a->accel_x = 0x04;
        a->flip = (uint8_t)(a->facing ^ 0x01);
        a->resume = ACROSS_SLOW;
        return 1;
    case ACROSS_SLOW:
        ActorMoveX(a);
        stops = slows_past_zero_x(a);
        ActorDecelerateX(a);
        if (!stops)
            return 1;
        a->facing ^= 0x01;
        a->resume = ACROSS_LEG;
        return 0x18;
    case ACROSS_LEG:
        return across_leg(a);

    case UPDOWN_SPEED:
        step_y(a);
        ActorAccelerateY(a);
        if ((uint16_t)a->speed_y >= SPEED_TOP)
            a->accel_y = 0;
        if (--k->frames)
            return 1;
        a->accel_y = 0x03;
        a->resume = UPDOWN_SLOW;
        return 1;
    case UPDOWN_SLOW:
        step_y(a);
        stops = slows_past_zero_y(a);
        ActorDecelerateY(a);
        if (!stops)
            return 1;
        a->facing ^= 0x01;
        return updown_leg(a);

    case RUN:
        ActorMoveX(a);
        ActorAccelerateX(a);
        if (--k->frames)
            return 1;
        AnimStart(a, 8);
        a->accel_x = 0;
        a->accel_y = 0x30;
        k->frames = 0x10;
        a->resume = RUN_RISE;
        return 1;
    case RUN_RISE:
        ActorStepYBack(a);
        ActorMoveX(a);
        ActorAccelerateY(a);
        if (--k->frames)
            return 1;
        a->resume = RUN_SLOW;
        return 1;
    case RUN_SLOW:
        ActorStepYBack(a);
        ActorMoveX(a);
        ActorDecelerateY(a);
        if (a->speed_y >= 0)
            return 1;
        a->resume = AWAY;
        return 1;
    /*
     * @routine 83:EE3B Kind6DAway
     * WHAT IT DOES: Way 2's end: across each frame, for good. No input
     * reaches it: the rise before it goes on while its vertical speed is
     * not negative, right after ActorDecelerateY, which never leaves it
     * negative.
     */
    case AWAY:
        ActorMoveX(a);
        return 1;

    case HOP_AIR:
        ActorFly(a);
        if (a->spawn_byte & 0x10) {
            if (!((uint16_t)a->speed_y & 0x8000) && (uint16_t)a->speed_y >= FALL_CUT)
                a->accel_y = 0;
        } else if (ActorOutOfWallProbe4(a)) {
            a->facing ^= 0x01;
            a->flip = a->facing;
        }
        if (!ActorOutOfGround(a))
            return 1;
        AnimStart(a, 2);
        a->resume = HOP_LAND;
        return 0x08;
    case HOP_LAND:
        AnimStart(a, 1);
        a->resume = HOP_AGAIN;
        return 0x08;
    case HOP_AGAIN:
        ActorMotionPreset(a, hop_preset(a));
        AnimStart(a, 8);
        a->resume = HOP_AIR;
        return 1;

    case HOME:
        ActorMoveByVector(a, 0x18);
        if (--k->frames)
            return 1;
        if (--k->turns) {
            dir = ActorDirectionToPlayer(a);
            diff = (uint8_t)((dir - a->facing) & 0x0F);
            if (diff) {
                a->facing = (uint8_t)((diff >= 0x08 ? a->facing - 1 : a->facing + 1) & 0x0F);
                a->flip = (uint8_t)(((a->facing + 4) & 0x0F) >> 3);
            }
            k->frames = 0x0C;
            return 1;
        }
        k->frames = 0x0C;
        k->turns = a->flip ? a->flip : 0xFF;
        home_up_turn(a);
        a->resume = HOME_UP;
        return 1;
    case HOME_UP:
        ActorMoveByVector(a, 0x18);
        home_up_turn(a);
        return 1;

    case SPAWN_WAIT:
        if (--k->frames)
            return 1;
        a->resume = SPAWN_FAR;
        return 1;
    case SPAWN_FAR:
        if (from_range_x(a) < SPAWN_NEAR)
            return 1;
        return spawn_try(a);

    /*
     * @routine 83:EFF0 Kind6DSecond
     * WHAT IT DOES: Kind $6D downed (its second entry): knocked away
     * (preset $1F) in pose 7, with sound $0F unless a shot downed it while
     * its facing's bit 7 was clear; that bit cleared; its animation 10
     * frames, then each frame its animation and flight until it is out of
     * range by box 0.
     */
    case ACTOR_START_SECOND:
        ActorMotionPreset(a, 0x1F);
        AnimStart(a, 7);
        k->frames = 0x0A;
        if (!k->downed || (a->facing & 0x80))
            SoundQueueAdd(0x0F);
        a->facing &= 0x7F;
        a->resume = DOWN_HOLD;
        return 1;
    case DOWN_HOLD:
        AnimStep(a);
        if (--k->frames)
            return 1;
        a->resume = DOWN_FLY;
        return 1;
    case DOWN_FLY:
        AnimStep(a);
        ActorFly(a);
        if (ActorEndOutOfRange(a, 0) == ACTOR_ENDED)
            return ACTOR_ENDED;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:EF92 Kind6DWait
 * WHAT IT DOES: Kind $6D's wait, by its way (+$07 bits 2-4): way 5 only
 * its end when out of range by box 0. Way 7: while the byte at $1B14
 * (g.boss_beaten) is set it enters state $8C (bits $8C set) with flags bit 7 cleared; stomped
 * or hurting (ActorStompOrHurt), a shot downs it (+$48 counted), its
 * animation; ended (ActorEndPlaced: the original's 83:96C6) once $90 or
 * more from the range point. The rest: stomped or hurting, downed by
 * shots, its animation, way 1 its height by its frame (ActorYByFrame); its
 * end when out of range by box 0.
 */
int Kind6DWait(Actor *a)
{
    K6DOwn *k = k6d(a);
    uint8_t way = (uint8_t)(a->spawn_byte & 0x1C);

    if (way == 0x1C) {
        /* the byte at $1B14 (state.h's boss_beaten: the record's other
           users of the byte, the chain head's word among them, share it) */
        if (g.boss_beaten) {
            a->state |= 0x8C;
            a->flags &= (uint8_t)~0x80;
        }
        (void)ActorStompOrHurt(a);
        if (ActorDownedByShot(a))
            k->downed++;
        AnimStep(a);
        if (from_range_x(a) >= SPAWN_NEAR)
            return ActorEndPlaced(a);
        return ACTOR_GOES_ON;
    }
    if (way != 0x14) {
        (void)ActorStompOrHurt(a);
        if (ActorDownedByShot(a))
            k->downed++;
        AnimStep(a);
        if (way == 0x04)
            ActorYByFrame(a);
    }
    return ActorEndOutOfRange(a, 0);
}
