/*
 * course_platform.c: actor kind $47, the platforms of level 5's carpet
 * course: the first one (+$07 0) is the course maker: it works the course
 * out as a list of waypoints, starts four platforms along it and keeps the
 * live stretch of it at the screen's middle; the platforms it starts fly
 * the course and carry Aladdin when he stands on one (it dips under his
 * weight). Its wait routine is PlatformWait. (Kind $62's second entry
 * goes on here too.)
 *
 * The course is the waypoint list at the original's $7F:5000: x and y (24
 * bits), the direction, the speed. The platforms' fields the original
 * reused are named for what they are here: the course's end (the maker's),
 * the dip, the last place, how far it moved. Some sit in fields of the
 * record that are named otherwise (its speed across and down, its pull, its
 * last direction); the course's offsets (the original's +$37) are a table
 * by record, as +$37 is Actor.next here.
 */
#include "actor/actor_own.h"
#include "video/view.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"
#include "data/kinds_d_tables.h"
#include "map/map.h"

/* its own bytes */
#define JUMPED OWN_16           /* counted up when it was moved along the course this frame */
#define SOUND OWN_17            /* the dip's sound */
#define DIP_PHASE OWN_13        /* what its dip is doing: an entry of the dip's phases */
#define RIDER_X OWN_35          /* the player's x pixels as the rider check found them (word) */
#define DIRECTION OWN_40        /* the direction it moves along the course: 0 or 1 */
#define DIRECTION_FLIP OWN_42   /* nonzero: its wait routine flips DIRECTION */
#define MOVED_X OWN_45          /* x pixels moved in the frame (word) */
#define MOVED_Y OWN_47          /* y pixels moved in the frame (word) */
#define RIDDEN OWN_49           /* nonzero: the player stands on it this frame */
/* how far it has sunk under the player: 24 bits, +$4A-$4C (own.bytes keeps
   +$4A-$4B after +$4F, so not in a row: see dip()) */
#define BASE_Y OWN_4D           /* its y without the dip: 24 bits (+$4D-$4F) */

#define COURSE_STEPS_APART 38   /* steps from one waypoint to the next */
#define COURSE_START_X 0x002A
#define COURSE_START_Y 0x01C0
#define COURSE_END_X_HIGH 0x10  /* the course is worked out to x $1000 */
#define COURSE_WAYPOINTS 4096   /* room for the course (the original's: from $7F:5000) */

typedef struct Waypoint {
    uint32_t x, y;              /* 24 bits each */
    uint8_t direction, speed;
} Waypoint;

static Waypoint course[COURSE_WAYPOINTS];
static unsigned course_end;     /* the maker's: the next waypoint to keep */
static unsigned course_live;    /* the waypoint the course is live around ($0400): the maker's */
static uint8_t course_steps;    /* the maker's: steps left before the next waypoint is kept (+$22) */
static unsigned course_at[ACTOR_RECORDS]; /* by record: the waypoint it is at (the original's +$37, in bytes: 8 a waypoint) */

enum {
    SLOW = ACTOR_RESUME_OWN,    /* each frame out of a turn stretch (PlatformSlowStretch) */
    TURN,                       /* each frame in one (PlatformTurnStretch) */
    MAKER                       /* the maker, each frame */
};

/* dip phases */
enum { DIP_IDLE, DIP_LAND, DIP_SINK, DIP_RISE, DIP_WAIT_OFF, DIP_SINK_AGAIN, DIP_RISE_END };

static unsigned index_of(const Actor *a) { return (unsigned)(a - g.actors); }

/* the waypoint `n`, a copy of nothing when past the list (the original reads
   whatever its memory holds there) */
static const Waypoint *waypoint(int n)
{
    static const Waypoint none;

    return n >= 0 && n < COURSE_WAYPOINTS ? &course[n] : &none;
}

static uint16_t own16(const Actor *a, unsigned at) { return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8); }

static void own16_set(Actor *a, unsigned at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

static uint32_t own24(const Actor *a, unsigned at)
{
    return (uint32_t)a->own.bytes[at] | (uint32_t)a->own.bytes[at + 1] << 8 | (uint32_t)a->own.bytes[at + 2] << 16;
}

static uint32_t dip(const Actor *a)
{
    return (uint32_t)a->own.bytes[OWN_4A] | (uint32_t)a->own.bytes[OWN_4B] << 8 | (uint32_t)a->own.bytes[OWN_4C] << 16;
}

static void dip_set(Actor *a, uint32_t v)
{
    a->own.bytes[OWN_4A] = (uint8_t)v;
    a->own.bytes[OWN_4B] = (uint8_t)(v >> 8);
    a->own.bytes[OWN_4C] = (uint8_t)(v >> 16);
}

static void own24_set(Actor *a, unsigned at, uint32_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
    a->own.bytes[at + 2] = (uint8_t)(v >> 16);
}

/* the original's fields of the platforms in fields of the record named
   otherwise: its speed along the course (+$20, its pull), the last direction
   (+$1C, its acceleration across), where it was at the frame's start (+$19
   and +$1D, its speeds) */
static uint8_t *course_speed(Actor *a) { return &a->accel_y; }
static uint8_t *last_direction(Actor *a) { return &a->accel_x; }
static uint16_t last_x(const Actor *a) { return (uint16_t)a->speed_x; }
static uint16_t last_y(const Actor *a) { return (uint16_t)a->speed_y; }

/*
 * @routine 83:CAC7 PlatformDipClear
 * WHAT IT DOES: Takes away the platform's dip: it is back at its height.
 */
static void dip_clear(Actor *a)
{
    dip_set(a, 0);
}

/*
 * @routine 83:CAC1 PlatformDipStart
 * WHAT IT DOES: Starts the platform's dip as the player lands on it: sound
 * `sound` kept for it, dip phase 1, the dip back to 0.
 */
static void dip_start(Actor *a, uint8_t sound)
{
    a->own.bytes[SOUND] = sound;
    a->own.bytes[DIP_PHASE] = DIP_LAND;
    dip_clear(a);
}

/*
 * @routine 83:CCBA PlatformDipStep
 * WHAT IT DOES: Moves the platform's dip by a frame's rate: sinking or
 * rising.
 */
static void dip_step(Actor *a, unsigned rising)
{
    dip_set(a, wrap24(dip(a) + k47_dip_rate(rising)));
}

/* the dip's pixels (its +$4B-$4C, a word) */
static uint16_t dip_pixels(const Actor *a) { return (uint16_t)(dip(a) >> 8); }

/*
 * @routine 83:CC19 PlatformDipUpdate
 * WHAT IT DOES: Runs the platform's dip for the frame: its phase's step,
 * then its y set to its height without the dip plus the dip. The phases
 * (83:CC18 PlatformDipIdle, 83:CC42 PlatformDipLand, 83:CC65
 * PlatformDipSink, 83:CC79 PlatformDipRise, 83:CC8D PlatformDipWaitOff,
 * 83:CC94 PlatformDipSinkAgain, 83:CCA8 PlatformDipRiseEnd): landed on, it
 * starts a puff (kind $4B, which sticks to it) with the landing's sound;
 * sinks to 11 pixels; rises to under 3; waits until he is off; sinks again
 * and rises back to its height.
 */
static void dip_update(Actor *a)
{
    switch (a->own.bytes[DIP_PHASE]) {
    case DIP_LAND: {
        Actor *puff = ActorSpawnFacing(a, 0x4B, 0, a->own.bytes[DIRECTION]);

        if (puff) {
            puff->next = a;
            if (a->flags & 0x40)
                SoundQueueAdd(a->own.bytes[SOUND]);
        }
        a->own.bytes[DIP_PHASE]++;
        break;
    }
    case DIP_SINK:
    case DIP_SINK_AGAIN:
        dip_step(a, 0);
        if (dip_pixels(a) >= 0x000B)
            a->own.bytes[DIP_PHASE]++;
        break;
    case DIP_RISE:
        dip_step(a, 1);
        if (dip_pixels(a) < 0x0003)
            a->own.bytes[DIP_PHASE]++;
        break;
    case DIP_WAIT_OFF:
        if (!a->own.bytes[RIDDEN])
            a->own.bytes[DIP_PHASE]++;
        break;
    case DIP_RISE_END:
        dip_step(a, 1);
        if (a->own.bytes[OWN_4C] & 0x80) {
            dip_clear(a);
            a->own.bytes[DIP_PHASE] = DIP_IDLE;
        }
        break;
    default:
        break;
    }
    a->y = wrap24(own24(a, BASE_Y) + dip(a));
}

/*
 * @routine 83:CB6D PlatformTurnZoneTest
 * WHAT IT DOES: Whether the platform is in a stretch of the course where it
 * turns: its x pixel inside one of the stretches (in order of x).
 */
static bool in_turn_zone(const Actor *a)
{
    uint16_t x = pixels(a->x);
    unsigned z;

    for (z = 0; z < k47_turn_zones(); z++) {
        if (x < k47_turn_from(z))
            return false;
        if (x < k47_turn_to(z))
            return true;
    }
    return false;
}

/*
 * @routine 83:CB24 PlatformCourseStep
 * WHAT IT DOES: Moves the platform a step along its direction at its course
 * speed, while the course is worked out; the step's y goes to its height
 * without the dip.
 */
static void course_step(Actor *a)
{
    DirectionStep step;

    a->y = own24(a, BASE_Y);
    k47_course_step(a->facing, step.x, step.y);
    ActorMoveScaled(a, &step, *course_speed(a));
    own24_set(a, BASE_Y, a->y);
}

/*
 * @routine 83:CAFC PlatformCourseSlower
 * WHAT IT DOES: A step of the course while it is worked out, slowing: the
 * course speed down by 1 (not below $20), the platform moved a step; true
 * when it is in a turn stretch. Also 83:CB10 PlatformCourseFaster: up by 1
 * (not above $80).
 */
static bool course_slower(Actor *a)
{
    uint8_t *speed = course_speed(a);

    *speed = (uint8_t)(*speed - 1) < 0x20 ? 0x20 : (uint8_t)(*speed - 1);
    course_step(a);
    return in_turn_zone(a);
}

static bool course_faster(Actor *a)
{
    uint8_t *speed = course_speed(a);

    *speed = (uint8_t)(*speed + 1) >= 0x80 ? 0x80 : (uint8_t)(*speed + 1);
    course_step(a);
    return in_turn_zone(a);
}

/*
 * @routine 83:CA3F PlatformCourseRecord
 * WHAT IT DOES: Keeps a waypoint of the platform's course every 38 steps
 * while the course is worked out: its place, direction and speed.
 */
static void course_record(Actor *a)
{
    if (--course_steps)
        return;
    course_steps = COURSE_STEPS_APART;
    if (course_end < COURSE_WAYPOINTS) {
        course[course_end].x = a->x;
        course[course_end].y = a->y;
        course[course_end].direction = a->facing;
        course[course_end].speed = *course_speed(a);
    }
    course_end++;
}

/*
 * @routine 83:CA11 PlatformSpawnOnCourse
 * WHAT IT DOES: Starts one more platform on the course: a child of kind
 * $47 at the waypoint the course maker's offset names; the next is six
 * waypoints on.
 */
static void spawn_on_course(Actor *maker, unsigned *at)
{
    Actor *child = ActorCreateChild(maker, 0x47);

    if (child) {
        const Waypoint *w = waypoint((int)*at);

        child->x = w->x;
        child->y = w->y;
        course_at[index_of(child)] = *at * 8;
    }
    *at += 6;
}

/* PlatformPositionKeep (83:CCD1): where it is at the frame's start, to
   measure later how far it moved. */
static void position_keep(Actor *a)
{
    a->speed_x = pixels(a->x);
    a->speed_y = pixels(a->y);
}

/*
 * @routine 83:CA75 PlatformOnCourseStart
 * WHAT IT DOES: Starts a platform the course maker put on the course:
 * nothing riding it, no dip, sprite bits $02 and $90, speed $20, its height
 * without the dip its y; then each frame it steps along the course.
 */
static int on_course_start(Actor *a)
{
    a->spawn_at = 0;
    a->own.bytes[DIRECTION] = 0;
    a->own.bytes[DIRECTION_FLIP] = 0;
    a->own.bytes[RIDDEN] = 0;
    dip_clear(a);
    *last_direction(a) = 0xFF;
    a->anim_flags = 0x02;
    a->flags = 0x90;
    *course_speed(a) = 0x20;
    own24_set(a, BASE_Y, a->y);
    a->facing = 0;
    a->resume = SLOW;
    return 1;
}

/* The course maker works the course out from its start to x $1000: slowing
   out of the turn stretches (facing 0), speeding through them (facing 1). */
static void work_out_course(Actor *a)
{
    for (;;) {
        a->facing = 0;
        for (;;) {
            if ((pixels(a->x) >> 8) == COURSE_END_X_HIGH)
                return;
            course_record(a);
            if (course_slower(a))
                break;
        }
        a->facing++;
        do
            course_record(a);
        while (course_faster(a));
    }
}

/*
 * @routine 83:C96E PlatformCourseStart
 * WHAT IT DOES: Starts a platform of kind $47. The first one (+$07 zero) is
 * the course maker: it works the course out from x $2A, y $1C0 to x $1000
 * (slowing out of the turn stretches, speeding through them, a waypoint
 * every 38 steps), starts four platforms along it from waypoint 6, six
 * waypoints apart, and from then on keeps the course's live waypoint at the
 * screen's middle as the play field scrolls. A platform it started (+$07
 * nonzero) goes on in PlatformOnCourseStart, then PlatformSlowStretch (each
 * frame, out of a turn stretch: kept in place, a step along the course
 * slowing; once a step puts it in one it asks for a direction flip and
 * faces 1) and PlatformTurnStretch (in one: speeding; once out, a flip, a
 * dip with sound $1E, faces 0).
 */
int PlatformCourseStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START: {
        int passes;
        unsigned at;

        if (a->spawn_byte)
            return on_course_start(a);
        *course_speed(a) = 0x20;
        a->x = with_pixels(a->x, COURSE_START_X);
        a->y = with_pixels(a->y, COURSE_START_Y);
        own24_set(a, BASE_Y, with_pixels(a->y, COURSE_START_Y));
        course_end = 0;
        course_steps = 1;
        work_out_course(a);
        passes = g.free_pool_top;
        if (passes > 0)
            (void)ActorFreeListSortHigh((uint8_t)passes);
        a->spawn_byte++;
        at = 6;
        spawn_on_course(a, &at);
        spawn_on_course(a, &at);
        spawn_on_course(a, &at);
        spawn_on_course(a, &at);
        course_live = 18;
        a->resume = MAKER;
        return 1;
    }
    case MAKER: {
        uint16_t scroll = pixels(g.layers[0].scroll_x), mid = (uint16_t)(scroll + 0x80 + view.dx);

        if (scroll >= g.scroll_x_before) {
            if (mid >= pixels(waypoint((int)course_live + 1)->x))
                course_live++;
        } else if (mid < pixels(waypoint((int)course_live)->x)) {
            course_live--;
        }
        return 1;
    }
    case SLOW:
        position_keep(a);
        if (!course_slower(a))
            return 1;
        a->own.bytes[DIRECTION_FLIP]++;
        a->facing = 1;
        a->resume = TURN;
        return 1;
    case TURN:
        position_keep(a);
        if (course_faster(a))
            return 1;
        a->own.bytes[DIRECTION_FLIP]++;
        dip_start(a, 0x1E);
        a->facing = 0;
        a->resume = SLOW;
        return 1;
    case ACTOR_START_SECOND:
    default:
        return 1;
    }
}

/*
 * @routine 83:CB93 PlatformCourseFollow
 * WHAT IT DOES: Follows the course: on to the next waypoint once the
 * platform passes its x; and when the platform is out of the stretch the
 * course keeps live (12 waypoints either side of the live one), moves it 24
 * waypoints on (or back, once the camera is left of it) with that
 * waypoint's place, direction and speed, its code going on by the
 * direction.
 */
static void course_follow(Actor *a)
{
    unsigned me = index_of(a);
    uint16_t x = pixels(a->x);
    int at = (int)(course_at[me] / 8), step;

    if (x < pixels(waypoint(at + 1)->x))
        return;
    course_at[me] += 8;
    at++;
    if (x >= pixels(waypoint((int)course_live - 12)->x) && x < pixels(waypoint((int)course_live + 12)->x))
        return;
    step = x >= g.range_x ? -24 : 24;
    at += step;
    if (at < 0)
        return;
    course_at[me] = (unsigned)at * 8;
    {
        const Waypoint *w = waypoint(at);

        a->x = w->x;
        a->y = w->y;
        own24_set(a, BASE_Y, w->y);
        *course_speed(a) = w->speed;
        a->own.bytes[DIRECTION] = w->direction;
        a->facing = w->direction;
        a->resume = (w->direction & 1) ? TURN : SLOW;       /* the table at $80:D312: SlowStretch, TurnStretch */
    }
    *last_direction(a) = 0xFF;
    a->own.bytes[DIRECTION_FLIP] = 0;
    a->own.bytes[DIP_PHASE] = DIP_IDLE;
    dip_clear(a);
    a->own.bytes[JUMPED]++;
}

/*
 * @routine 83:CD5E PlatformCarriesRecordY
 * WHAT IT DOES: Whether the player stands on the platform, by its box 0:
 * then his feet are put on its top at his place along it and he is carried
 * by its move in the frame.
 */
static bool carries(Actor *a, Actor *p)
{
    PlatformBox box = k47_box(0);
    uint16_t px = pixels(p->x), py2 = (uint16_t)(pixels(p->y) + 2), d, top, s2, s4, s8, s10, s0;
    unsigned c;

    if (!p->state)
        return false;
    c = last_x(a) >= px;
    d = (uint16_t)(last_x(a) - px + box.x_off + c);
    if (d >= box.width)
        return false;
    c = last_y(a) >= py2;
    d = (uint16_t)(last_y(a) - py2 + box.y_off + c);
    if (d >= box.height)
        return false;
    s0 = (uint16_t)(last_x(a) - box.x_off);
    s2 = s8 = (uint16_t)((uint16_t)(px - s0) >> 1);
    if (!a->own.bytes[DIRECTION] && !a->own.bytes[OWN_41])
        s2 = 0x10;
    s10 = (uint16_t)(last_y(a) - 0x10);
    s4 = (uint16_t)(s10 + s2);
    top = (uint16_t)(py2 + box.top_off - s4);
    if ((uint16_t)(py2 + box.top_off) < s4 || top >= 0x10)
        return false;
    if (own16(a, DIRECTION_FLIP)) {
        if ((own16(a, DIRECTION) ^ 1) == 0)
            s8 = 0x10;
        s4 = (uint16_t)(s10 + s8);
    }
    p->y = with_pixels(p->y, (uint16_t)(s4 - box.top_off + own16(a, MOVED_Y)));
    if (!g.kind66_touched)
        p->x = with_pixels(p->x, (uint16_t)(pixels(p->x) + own16(a, MOVED_X)));
    return true;
}

/*
 * @routine 83:CCF1 PlatformRiderCheck
 * WHAT IT DOES: Carries the player along on the platform when he stands on
 * it, unless that puts him into firm ground, and starts its dip under him
 * (sound $1F).
 */
static void rider_check(Actor *a)
{
    Actor *p = &g.actors[0];
    uint16_t kept;

    a->own.bytes[RIDDEN] = 0;
    if (!(a->flags & 0x40) || g.player_no_actor_landing || (g.player_in_air && ((p->speed_y >> 16) & 0x80)))
        return;
    kept = pixels(p->x);
    own16_set(a, RIDER_X, kept);
    if (!carries(a, p))
        return;
    g.player_on_actor++;
    a->own.bytes[RIDDEN]++;
    if (PlayerOnFirmGroundProbe8() || PlayerOnFirmGroundProbe4() || PlayerOnFirmGround())
        p->x = with_pixels(p->x, own16(a, RIDER_X));
    if (!a->own.bytes[DIP_PHASE])
        dip_start(a, 0x1F);
}

/*
 * @routine 83:CAD1 PlatformWait
 * WHAT IT DOES: The course platform's wait routine, each frame: its dip,
 * how far it moved (PlatformMoveMeasure), its rider, a direction flip when
 * its DIRECTION_FLIP is set, the course followed, and its animation by
 * direction; its animation steps. (83:CB8F LoneReturnCB8F is a lone RTS no
 * code names.)
 */
int PlatformWait(Actor *a)
{
    a->own.bytes[JUMPED] = 0;
    dip_update(a);
    own16_set(a, MOVED_X, (uint16_t)(pixels(a->x) - last_x(a)));
    own16_set(a, MOVED_Y, (uint16_t)(pixels(a->y) - last_y(a)));
    rider_check(a);
    if (a->own.bytes[DIRECTION_FLIP]) {
        a->own.bytes[DIRECTION_FLIP] = 0;
        a->own.bytes[DIRECTION] ^= 0x01;
    }
    course_follow(a);
    if (a->own.bytes[DIRECTION] != *last_direction(a)) {
        *last_direction(a) = a->own.bytes[DIRECTION];
        AnimStart(a, a->own.bytes[DIRECTION]);
    }
    AnimStep(a);
    return ACTOR_GOES_ON;
}
