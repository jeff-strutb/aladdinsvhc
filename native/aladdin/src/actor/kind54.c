/*
 * kind54.c: actor kinds $54 and $55, things that leap from a spot (kind
 * $54 straight up, kind $55 in an arc), trailing copies of themselves, and
 * come back down to leap again; with +$07 bit 3 set, one that only drops
 * and ends; and their waits.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: the drop's frames left */
    int8_t last_top;            /* +$41: its speed down's top byte last frame */
    uint8_t trail;              /* +$42: frames to its next trailing copy */
    uint8_t home_a[2];          /* +$43: kind $54: its y at rest; kind $55: its x (words) */
    uint8_t home_b[2];          /* +$45: kind $55: its y at rest */
} Kind54Own;

_Static_assert(sizeof(Kind54Own) <= OWN_BYTES, "a view fits the own bytes");

static inline Kind54Own *own(Actor *a) { return (Kind54Own *)a->own.bytes; }
static uint16_t get16(const uint8_t *w) { return (uint16_t)(w[0] | w[1] << 8); }
static void put16(uint8_t *w, uint16_t v) { w[0] = (uint8_t)v; w[1] = (uint8_t)(v >> 8); }

#define DROPS 0x08              /* +$07 bit 3: it only drops (and the copies it trails have it) */
#define LEAPING 0x04            /* +$07 bit 2: set while it leaps (for the kind $5E it spawns) */

enum {
    DROP_GO = ACTOR_RESUME_OWN, /* after the drop's first 7 frames */
    DROP,                       /* each of its 14 frames */
    K54_GO,                     /* kind $54: after 6 frames at the start of a leap */
    K54_FALL,                   /* each frame of the leap */
    LANDED,                     /* the frame after it lands */
    K54_AGAIN,                  /* after its 64 frames at rest */
    K55_START,                  /* kind $55: after its first 16 frames */
    K55_GO,
    K55_FLY,
    K55_AGAIN,
};

/* the drop (83:D7A5, Kind54Drop): 7 frames, pose 1, 14 frames falling, then it ends */
static int drop(Actor *a)
{
    a->anim_flags |= 0x03;
    ActorMotionReset(a, 0x08);
    own(a)->frames = 0x0E;
    a->resume = DROP_GO;
    return 7;
}

/* a leap's start: +$07 bit 2 set, a kind $5E, 6 frames */
static int leap_start(Actor *a, int next)
{
    a->spawn_byte |= LEAPING;
    ActorSpawn5E(a);
    a->resume = (uint16_t)next;
    return 6;
}

/* each frame up and down: the pose back at 0 as it turns to fall; a
   trailing copy every 8 frames */
static void leap_frame(Actor *a)
{
    int8_t top = (int8_t)((uint32_t)a->speed_y >> 16);

    if (top >= 0 && own(a)->last_top < 0)
        AnimStart(a, 0x00);
    own(a)->last_top = top;
    if (!--own(a)->trail) {
        own(a)->trail = 0x08;
        ActorSpawnCopy(a);
    }
}

/* down at its rest: +$07 bit 2 cleared, a kind $5E, sound $26, a frame */
static int land(Actor *a)
{
    a->spawn_byte &= (uint8_t)~LEAPING;
    ActorSpawn5E(a);
    SoundQueueAdd(0x26);
    a->resume = LANDED;
    return 1;
}

/* the start of both kinds' leapers: +$08 bit 7 set and bit 3 cleared, the
   first copy after $18 frames */
static void leaper(Actor *a)
{
    own(a)->trail = 0x18;
    a->anim_flags = (uint8_t)((a->anim_flags | 0x80) & ~0x08);
}

/* the leap's pose and sound (both kinds) */
static void leap_go(Actor *a)
{
    a->anim_flags = (uint8_t)((a->anim_flags | 0x08) & ~0x80);
    AnimStart(a, 0x02);
}

/*
 * @routine 83:D6D7 Kind54Start
 * WHAT IT DOES: Actor kind $54: leaps straight up from where it is placed
 * (speed by +$07 bits 0-1, gravity $20, sound $25), trailing a copy every 8
 * frames, and once back down at its start, sound $26, a pause of 64
 * frames, and up again; with +$07 bit 3 set it drops for 14 frames and
 * ends. A copier's cartridge RAM (written and read back at $70:3000) would
 * make it hold Aladdin's y at its own each time it turns to fall; the
 * retail board reads open bus there, so it never does (plan section 1,
 * item 8: not converted). Its second entry (83:D7F7) ends it.
 */
int Kind54Start(Actor *a)
{
    Kind54Own *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        a->own.bytes[OWN_4F] = 0;       /* the copier test's answer on the retail board */
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        if (a->spawn_byte & DROPS)
            return drop(a);
        a->probes = k54_probes();
        put16(o->home_a, pixels(a->y));
        leaper(a);
        return leap_start(a, K54_GO);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case DROP_GO:
        AnimStart(a, 0x01);
        a->resume = DROP;
        return 1;
    case DROP:
        ActorFall(a);
        if (--o->frames)
            return 1;
        return ActorEndPlaced(a);
    case K54_GO:
        leap_go(a);
        ActorSpeedYSet(a, k54_rise((uint8_t)(a->spawn_byte & 0x03)));
        a->accel_y = 0x20;
        SoundQueueAdd(0x25);
        a->resume = K54_FALL;
        return 1;
    case K54_FALL:
        ActorFall(a);
        leap_frame(a);
        if (get16(o->home_a) >= pixels(a->y))
            return 1;
        a->y = with_pixels(a->y, get16(o->home_a));
        return land(a);
    case LANDED:
        a->anim_flags = (uint8_t)((a->anim_flags | 0x80) & ~0x08);
        a->resume = a->kind == 0x55 ? K55_AGAIN : K54_AGAIN;
        return 0x40;
    case K54_AGAIN:
        return leap_start(a, K54_GO);
    default:
        return 1;
    }
}

/*
 * @routine 83:D7FB Kind55Start
 * WHAT IT DOES: Actor kind $55: (when +$07 bits 0, 1 and 3 are all clear,
 * after 16 frames) leaps in an arc from where it is placed (across by +$07
 * bits 0-1, turned for bit 4; up $96 for variant 2, $3F otherwise; gravity
 * $16; sound $25), trailing a copy every 8 frames; once back down at its
 * start's height, sound $26, a pause of 64 frames, put back at its start,
 * and again. With +$07 bit 3 set it drops and ends as kind $54's does.
 * Its second entry (83:D8DE) ends it.
 */
int Kind55Start(Actor *a)
{
    Kind54Own *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        if (!(a->spawn_byte & 0x0B)) {
            a->resume = K55_START;
            return 0x10;
        }
        /* fall through */
    case K55_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        if (a->spawn_byte & DROPS)
            return drop(a);
        a->probes = k55_probes();
        put16(o->home_a, pixels(a->x));
        put16(o->home_b, pixels(a->y));
        leaper(a);
        if (a->spawn_byte & 0x10)
            a->facing++;
        return leap_start(a, K55_GO);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case K55_GO: {
        uint8_t way = (uint8_t)(a->spawn_byte & 0x03);

        leap_go(a);
        ActorSpeedXSet(a, k55_across(way));
        a->accel_y = 0x16;
        ActorSpeedYSet(a, way == 2 ? 0x96 : 0x3F);
        SoundQueueAdd(0x25);
        a->resume = K55_FLY;
        return 1;
    }
    case K55_FLY:
        ActorFly(a);
        leap_frame(a);
        if (get16(o->home_b) >= pixels(a->y))
            return 1;
        return land(a);
    case K55_AGAIN:
        a->x = with_pixels(a->x, get16(o->home_a));
        a->y = with_pixels(a->y, get16(o->home_b));
        return leap_start(a, K55_GO);
    default:
        /* DROP_GO, DROP and LANDED are kind $54's code's */
        return Kind54Start(a);
    }
}

/*
 * @routine 83:D8CE ActorWaitHurtOnTouch
 * WHAT IT DOES: Kind $55's wait: its animation on; unless bit 3 of its
 * +$07 is set, it hurts Aladdin on touch; it is ended once left behind.
 * Kind $54's wait is the same code:
 * @routine 83:D7E7 Kind54Wait
 */
int ActorWaitHurtOnTouch(Actor *a)
{
    AnimStep(a);
    if (!(a->spawn_byte & DROPS))
        (void)ActorHurtsPlayerOnTouch(a);
    return ActorEndIfLeftBehind(a);
}
