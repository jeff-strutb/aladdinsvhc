/*
 * kind7e.c: actor kind $7E, the platforms Aladdin bounces on: by its +$07
 * one rises and falls in turn, drifts across and back aslant, or is the
 * hidden centre of two or three that circle it; and kind $7F, a backdrop
 * figure that drifts across for good (kind $7E's second entry is kind
 * $7F's start, so both are here). Their waits are here too: kind $7E's
 * bounce, kind $7F's animation.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

#define K7E_GRAVITY 0x06        /* its vertical acceleration (ActorMotionReset) */
#define K7E_TOP_SPEED 0xC0      /* the vertical speed's fraction at which it stops speeding up */
#define K7E_TURN_WAIT 0x20      /* frames it waits each time it turns */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t down;               /* +$40: 0 it moves against its vertical speed, else with it; kind $7F: +$07 * 16 */
    uint8_t left;               /* +$41: frames left of its glide; a centre: children left to make, less one */
    uint8_t bounce;             /* +$42: frames left of a bounce (the player held on it) */
    uint8_t centre_x[2];        /* +$43-$44: a circler: the x it circles (pixels) */
    uint8_t centre_y[2];        /* +$45-$46: and the y */
    uint8_t stays;              /* +$47: nonzero: never freed out of range (a circler) */
    uint8_t radius;             /* +$48: a circler's distance from its centre; a centre's for its children */
    uint8_t angle_low;          /* +$49: a circler: its direction from the centre, 0-$1FF (low byte) */
    uint8_t centre_high;        /* +$4C: its centre's index in g.actors, high byte */
    uint8_t turn;               /* +$4D: a circler: 0 its angle counts up, 1 down */
    uint8_t glide;              /* +$4E: frames it glides at full speed */
    uint8_t next;               /* +$4F: a centre: the entry of the angles its next child starts at */
    uint8_t angle_high;         /* +$4A */
    uint8_t centre_low;         /* +$4B: its centre's index in g.actors, low byte */
} Kind7EOwn;

_Static_assert(sizeof(Kind7EOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind7EOwn *own(Actor *a) { return (Kind7EOwn *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }
static uint16_t angle(Kind7EOwn *o) { return (uint16_t)(o->angle_low | o->angle_high << 8); }
static void set_angle(Kind7EOwn *o, uint16_t v) { o->angle_low = (uint8_t)v; o->angle_high = (uint8_t)(v >> 8); }
static Actor *centre(Kind7EOwn *o) { return &g.actors[o->centre_low | o->centre_high << 8]; }

enum {
    SPEED_UP = ACTOR_RESUME_OWN,    /* each frame of speeding up */
    GLIDE,                          /* each frame of its glide */
    SLOW,                           /* each frame of slowing down */
    TURNED,                         /* after its turn's 32 frames */
    RING_WAIT,                      /* a centre: each frame until out of range */
    CIRCLE,                         /* a circler: each frame */
    DRIFT,                          /* kind $7F's drift: each frame */
};

/*
 * @routine 84:92BC Kind7EStep
 * WHAT IT DOES: Kind $7E's step: across by its speed the way it faces, and
 * up or down by its vertical speed as its +$40 says.
 */
static void step(Actor *a)
{
    ActorMoveX(a);
    if (!own(a)->down)
        ActorStepYBack(a);
    else
        ActorStepY(a);
}

/*
 * @routine 84:92CE Kind7EShow
 * WHAT IT DOES: Shows a kind $7E with animation 0, its wait (the bounce)
 * to run each frame it waits.
 */
static void show(Actor *a)
{
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    AnimStart(a, 0);
}

/* A frame of its rise and fall (84:91D1 on, for good): speed up to the
   top speed, glide, slow to a stop, turn and wait 32 frames; again. */
static int rise_and_fall(Actor *a)
{
    Kind7EOwn *o = own(a);

    switch (a->resume) {
    case SPEED_UP:
        ActorAccelerateX(a);
        ActorAccelerateY(a);
        if ((uint8_t)a->speed_y < K7E_TOP_SPEED) {
            step(a);
            return 1;
        }
        o->left = o->glide;
        a->resume = GLIDE;
        return 1;
    case GLIDE:
        step(a);
        if (--o->left)
            return 1;
        a->resume = SLOW;
        return 1;
    default:                        /* SLOW */
        ActorDecelerateX(a);
        ActorDecelerateY(a);
        if ((uint8_t)a->speed_y) {
            step(a);
            return 1;
        }
        o->down ^= 1;
        a->facing ^= 1;
        a->resume = TURNED;
        return K7E_TURN_WAIT;
    }
}

/* +$07 0, and 5 or 6 aslant (acceleration across 3): shown, still with
   gravity 6, then the rise and fall. */
static int rise(Actor *a, uint8_t glide, int aslant)
{
    show(a);
    if (!aslant)
        ActorMotionReset(a, K7E_GRAVITY);
    own(a)->down = 0;
    own(a)->bounce = 0;
    own(a)->glide = glide;
    if (aslant) {
        ActorMotionReset(a, K7E_GRAVITY);
        a->accel_x = 0x03;
    }
    a->resume = SPEED_UP;
    return 1;
}

/*
 * @routine 84:92DD Kind7ERing
 * WHAT IT DOES: Makes a kind $7E centre's children, its +$41 + 1 of them,
 * each to circle the centre's place at its radius from its own first
 * angle, its look the centre's.
 */
static void ring(Actor *a)
{
    Kind7EOwn *o = own(a);

    do {
        Actor *c = ActorSpawn(a, a->kind);

        if (c) {
            Kind7EOwn *k = own(c);
            uint16_t me = (uint16_t)(a - g.actors);

            c->look = a->look;
            k->centre_low = (uint8_t)me;
            k->centre_high = (uint8_t)(me >> 8);
            set_angle(k, k7e_first_angle(o->next));
            set_word(k->centre_x, pixels(a->x));
            set_word(k->centre_y, pixels(a->y));
            k->radius = o->radius;
        }
        o->next--;
    } while ((int8_t)--o->left >= 0);
}

/* +$07 1-4: the children of kind $7E (+$07 7 or 8, 3 or 2 of them at
   radius $B2 or $74), then unseen until out of range. */
static int centre_start(Actor *a, uint8_t radius, uint8_t left, uint8_t next)
{
    own(a)->radius = radius;
    own(a)->left = left;
    own(a)->next = next;
    ring(a);
    a->resume = RING_WAIT;
    return 1;
}

/* +$07 7 and 8: circles its centre's place, one way or the other, until
   the centre's record is free. */
static int circle_frame(Actor *a)
{
    Kind7EOwn *o = own(a);
    uint16_t v;

    if (!centre(o)->state)
        return ActorEndPlaced(a);
    a->x = (uint32_t)word(o->centre_x) << 8;
    a->y = (uint32_t)word(o->centre_y) << 8;
    ActorMoveAlong(a, angle(o), o->radius);
    v = (uint16_t)(angle(o) + 1);
    if (o->turn & 1)
        v = (uint16_t)(v - 2);
    set_angle(o, (uint16_t)(v & 0x01FF));
    return 1;
}

/*
 * @routine 84:934B Kind7FDrift
 * WHAT IT DOES: Actor kind $7F (and kind $7E's second entry): shown with
 * the animation its +$07 picks (bits of +$07 also in +$08), facing right,
 * it drifts across at $70 less 16 times +$07 (a fraction of a pixel a
 * frame) for good.
 */
static int drift(Actor *a)
{
    uint8_t variant = a->spawn_byte;

    a->anim_flags |= (uint8_t)(0x02 | variant);
    a->flags |= 0x90;
    a->probes = k7f_probes();
    AnimStart(a, variant);
    a->facing = 0;
    a->flip = 0;
    own(a)->down = (uint8_t)(variant << 4);
    a->speed_x = (a->speed_x & ~0xFF) | (uint8_t)(0x70 - own(a)->down);
    a->resume = DRIFT;
    return 1;
}

/*
 * @routine 84:91A5 Kind7EStart
 * @routine 83:92A8 Kind7EEnter
 * WHAT IT DOES: Actor kind $7E (the loop's stub 83:92A8 jumps here), a
 * platform Aladdin bounces on, by its +$07: 0 rises, glides, slows to a
 * stop, waits 32 frames and comes back down, and so on for good; 5 and 6
 * do the same aslant, rightward or leftward first; 1 to 4 make two or three
 * children of kind $7E (+$07 7 or 8) set round it, and stay unseen where
 * they were placed until out of range; 7 and 8 (the children) circle their
 * centre's place, one way or the other, until the centre's record is free.
 * Past 8 the original jumps through the words after its table of ways (no
 * placement has one): said once, and still.
 */
static int start(Actor *a)
{
    own(a)->stays = 0;
    switch (a->spawn_byte) {
    case 0:
        return rise(a, 0x36, 0);
    case 1:
    case 2:
        a->spawn_byte = a->spawn_byte == 1 ? 0x07 : 0x08;
        return centre_start(a, 0xB2, 0x02, 0x02);
    case 3:
    case 4:
        a->spawn_byte = a->spawn_byte == 3 ? 0x07 : 0x08;
        return centre_start(a, 0x74, 0x01, 0x04);
    case 5:
    case 6:
        a->facing = (uint8_t)(a->spawn_byte - 5);
        return rise(a, 0x44, 1);
    case 7:
    case 8:
        own(a)->turn = (uint8_t)(a->spawn_byte - 7);
        show(a);
        own(a)->bounce = 0;
        own(a)->stays++;
        a->resume = CIRCLE;
        return 1;
    default:
        return kind_lost(a, "Kind7EStart");
    }
}

int Kind7ECode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return drift(a);
    case SPEED_UP:
    case GLIDE:
    case SLOW:
        return rise_and_fall(a);
    case TURNED:
        a->resume = SPEED_UP;
        return 1;
    case RING_WAIT:
        if (ActorEndOutOfRange(a, 0x00) == ACTOR_ENDED)
            return ACTOR_ENDED;
        return 1;
    case CIRCLE:
        return circle_frame(a);
    case DRIFT:
        ActorMoveX(a);
        return 1;
    default:
        return kind_lost(a, "Kind7ECode");
    }
}

/* Kind $7F's code: its drift; its second entry (84:9383) a JML to
   ActorEndPlaced. */
int Kind7FCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return drift(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case DRIFT:
        ActorMoveX(a);
        return 1;
    default:
        return kind_lost(a, "Kind7FCode");
    }
}

/*
 * @routine 83:92AC ActorKind7EWait
 * @routine 84:931C Kind7EWaitWork
 * WHAT IT DOES: Kind $7E's wait routine (the loop's stub JSLs the work):
 * for 8 frames after a bounce the player is held 38 pixels above it; else
 * coming down on it bounces him (sound $12); unless +$47, freed once out of
 * range by box 8.
 */
int Kind7EWait(Actor *a)
{
    Kind7EOwn *o = own(a);

    if (o->bounce) {
        o->bounce--;
        /* SBC #$25 with the carry clear: 38 */
        g.actors[0].y = with_pixels(g.actors[0].y, (uint16_t)(pixels(a->y) - 0x26));
    } else if (ActorBouncesPlayer(a)) {
        SoundQueueAdd(0x12);
        o->bounce = 8;
    }
    if (o->stays)
        return ACTOR_GOES_ON;
    return ActorFreeOutOfRange(a, 0x08) ? ACTOR_ENDED : ACTOR_GOES_ON;
}

/*
 * @routine 83:92B9 ActorKind7FWait
 * @routine 84:9387 KindAnimateFreeOff
 * WHAT IT DOES: Kind $7F's wait routine (the loop's stub JSLs the work): a
 * frame of its animation, then freed once out of range by box $18.
 */
int Kind7FWait(Actor *a)
{
    AnimStep(a);
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
