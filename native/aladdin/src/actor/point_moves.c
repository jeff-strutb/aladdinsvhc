/*
 * point_moves.c: actor helpers that move an actor to a point, rise or hop
 * over frames. The original's keep their caller's return address (at
 * +$4E) while they wait, so the actor's stack is as the loop expects at
 * each BRK; here a caller calls the helper again at the same resume point
 * each turn until it returns ACTOR_RETURNS (actor.h), and the helper's
 * place is a->call_at.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"
#include "map/map.h"

enum { INSIDE = 1 };                    /* a->call_at: past the first frame */

static uint16_t own16(const Actor *a, int at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

/* the point ActorGoToPoint goes to, its direction set, and ActorSpawnLookWait's kind */
#define GOTO_X OWN_48
#define GOTO_Y OWN_4A
#define GOTO_SET OWN_4C
#define SPAWN_KIND OWN_4D
#define RISE_LIMIT OWN_45               /* the rise's top speed (word) */

/*
 * @routine 83:FC5D ActorGoToPoint
 * WHAT IT DOES: Moves the actor toward the point at its +$48, +$4A
 * (pixels) a step a frame, facing the way it goes (the direction ActorAimAt
 * gives), by its direction set +$4C, until it is inside box $20 round the
 * point; then it goes back to its caller (ACTOR_RETURNS).
 * Also 83:FB4F ActorBackToCaller, which puts the caller's return address
 * back: here the return itself.
 */
int ActorGoToPoint(Actor *a)
{
    uint16_t x = own16(a, GOTO_X), y = own16(a, GOTO_Y);

    if (a->call_at && ActorNearPoint(a, 0x20, x, y)) {
        a->call_at = 0;
        return ACTOR_RETURNS;
    }
    a->facing = ActorAimAt(a, x, y);
    ActorMoveByVector(a, a->own.bytes[GOTO_SET]);
    a->call_at = INSIDE;
    return 1;
}

/* whether Aladdin is inside the reach of the table's entry `at` (a reach of
   0 is not tested): across, his x within the reach either side of its; down,
   his y within the reach below its. 83:FCF5 PlayerInReach. */
static bool in_reach(const Actor *a, const ReachEntry *e)
{
    uint16_t x = pixels(g.actors[0].x), y = pixels(g.actors[0].y);

    if (e->across && (uint16_t)(pixels(a->x) + e->across - x) >= (uint16_t)(e->across << 1))
        return false;
    if (e->down && (uint16_t)(e->down + pixels(a->y) - y) >= e->down)
        return false;
    return true;
}

/*
 * @routine 83:FCA5 ActorRiseWhenInReach
 * WHAT IT DOES: When Aladdin is in reach by the reach table's entry `at`
 * (PlayerInReach: 83:FCF5), the actor goes down a frame at a time, faster
 * each frame up to the entry's top speed, until it is out of the ground
 * (ActorOutOfGround); then back to its caller (ACTOR_RETURNS). Out of reach,
 * back at once with ACTOR_RETURNS_C.
 * The speed is its +$1D (a->speed_y, a positive word), the step its +$20
 * (a->accel_y), the top speed its +$45.
 */
int ActorRiseWhenInReach(Actor *a, uint8_t at)
{
    ReachEntry e = reach_entry(at);
    uint16_t speed;

    if (!a->call_at) {
        if (!in_reach(a, &e))
            return ACTOR_RETURNS_C;
        a->own.bytes[RISE_LIMIT] = (uint8_t)(e.limit << 4);
        a->own.bytes[RISE_LIMIT + 1] = (uint8_t)(e.limit >> 4);
        a->speed_y = (int32_t)((uint32_t)e.speed << 4);
        a->accel_y = e.step;
        a->call_at = INSIDE;
        return 1;
    }
    speed = (uint16_t)(a->accel_y + (uint16_t)a->speed_y);
    if (speed >= own16(a, RISE_LIMIT))
        speed = own16(a, RISE_LIMIT);
    a->speed_y = speed;
    ActorMoveY(a);
    if (!ActorOutOfGround(a))
        return 1;
    a->call_at = 0;
    return ACTOR_RETURNS;
}

/*
 * @routine 83:FC0D ActorSpawnLookWait
 * WHAT IT DOES: Spawns an actor of the kind in its +$4D next to the actor,
 * with the actor's look, waiting a frame and trying again while no record
 * is free; then back to its caller (ACTOR_RETURNS).
 */
int ActorSpawnLookWait(Actor *a)
{
    if (ActorSpawnSameLook(a, a->own.bytes[SPAWN_KIND]))
        return ACTOR_RETURNS;
    return 1;
}

/*
 * @routine 83:FD2C ActorHopFromTable
 * WHAT IT DOES: A hop by the hop table's entry `at`: its gravity, vertical
 * speed (up) and horizontal speed set; each frame it moves across, falls
 * and is put out of a wall (probe 4), and once it is falling, until it is
 * out of the ground again; then back to its caller (ACTOR_RETURNS).
 */
int ActorHopFromTable(Actor *a, uint8_t at)
{
    if (!a->call_at) {
        HopEntry e = hop_entry(at);

        a->accel_y = e.gravity;
        a->speed_y = -0x10000 + e.up;       /* the third byte $FF: up (24 bits signed) */
        a->speed_x = e.across;
        a->call_at = INSIDE;
        return 1;
    }
    ActorMoveX(a);
    ActorFall(a);
    (void)ActorOutOfWallProbe4(a);
    if (a->speed_y < 0 || !ActorOutOfGround(a))
        return 1;
    a->call_at = 0;
    return ACTOR_RETURNS;
}

/*
 * @routine 83:FD6D ActorProbeAheadInGround
 * WHAT IT DOES: Tests two points for ground: `ahead` in front of the actor
 * (by its facing) and `up` above it, then `up` above its own x: true when
 * either's tile has a behaviour kind (bits 0-2), whatever the point's place
 * in it.
 */
bool ActorProbeAheadInGround(const Actor *a, uint16_t ahead, uint16_t up)
{
    MapPoint p = { 0 };
    uint16_t x = pixels(a->x);

    p.x = (uint16_t)(x + (a->facing ? -ahead : ahead));
    p.y = (uint16_t)(pixels(a->y) - up);
    (void)PointInGround(a, &p);
    if (p.behaviour & 0x07)
        return true;
    p.x = x;
    (void)PointInGround(a, &p);
    return (p.behaviour & 0x07) != 0;
}
