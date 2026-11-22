/*
 * kind86.c: actor kind $86 (act 5, stage 0), a pair that drops and rises
 * again: the placed one (+$07 bit 2 clear) and its child (bit 2 set) kept
 * $2B pixels above it. The player standing on the placed one rides with
 * it; the pair can hold him, and its zone hurts him.
 */
#include "actor/actor_own.h"
#include "video/view.h"
#include "audio/sound.h"

/* its own bytes */
typedef struct {
    uint8_t before_13[OWN_13];
    uint8_t hold_at;            /* +$13: ActorHoldPlayer's offset for the player */
    uint8_t before_40[OWN_40 - OWN_13 - 1];
    uint8_t count;              /* +$40: frames left of each step of its cycle */
    uint8_t unused_41_42[2];
    uint8_t home_y[2];          /* +$43-$44: its y pixels as it started: the rise ends above it */
    uint8_t unused_45_46[2];
    uint8_t last_y[2];          /* +$47-$48: its y pixels as this frame began */
    uint8_t stood;              /* +$49: nonzero, the player stands on it this frame */
    uint8_t unused_4c_4e[3];
    uint8_t own_4f;             /* +$4F: cleared as it starts (ActorHoldPlayer's pause) */
    uint8_t left;               /* +$4A: the drop's frames left (1: its zone hurt him, the drop ends) */
    uint8_t pair;               /* +$4B: the placed one: its child, as its index in g.actors (0 none) */
} K86Own;

_Static_assert(sizeof(K86Own) == OWN_BYTES, "kind $86's view follows own.bytes");

static inline K86Own *k86(Actor *a) { return (K86Own *)a->own.bytes; }
static inline uint16_t get16(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static inline void set16(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

enum {
    CHILD = ACTOR_RESUME_OWN,   /* the child: it only animates */
    SPAWNED,                    /* the placed one, a frame after making its child */
    HOLD,                       /* 16 frames */
    DROP,                       /* its frames of the drop */
    BOUNCE_1,                   /* 2 frames */
    BOUNCE_UP,                  /* a frame, a pixel up */
    BOUNCE_2,                   /* 2 frames */
    BOUNCE_DOWN,                /* a frame, the pixel back */
    STILL,                      /* $40 frames */
    RISE                        /* until back above where it started */
};

#define ABOVE 0x2B              /* the child's height above it, pixels */
#define REACH_X 0x00E0u         /* the box round the range point it stays in: half across, across */
#define SPAN_X 0x01C0u
#define REACH_Y 0x0100u         /* half up and down, up and down */
#define SPAN_Y 0x0200u

static Actor *child_of(Actor *a)
{
    uint8_t n = k86(a)->pair;

    return n ? &g.actors[n] : NULL;
}

/*
 * @routine 84:979B Kind86HeightKeep
 * WHAT IT DOES: Keeps its height as the frame begins, and clears its
 * "stood on".
 */
static void Kind86HeightKeep(Actor *a)
{
    set16(k86(a)->last_y, pixels(a->y));
    k86(a)->stood = 0;
}

/*
 * @routine 84:97A6 Kind86Carry
 * WHAT IT DOES: The player standing on it moves with it: his y by its
 * move since the frame began.
 */
static void Kind86Carry(Actor *a)
{
    Actor *p = &g.actors[0];

    if (!k86(a)->stood)
        return;
    p->y = with_pixels(p->y, (uint16_t)(pixels(a->y) - get16(k86(a)->last_y) + pixels(p->y)));
}

/*
 * @routine 84:97CB Kind86StandCheck
 * WHAT IT DOES: Lets the player stand on it, 4 pixels above its place
 * (box $24), and marks it stood on when he does.
 */
static void Kind86StandCheck(Actor *a)
{
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 4));
    if (ActorSolidForPlayer(a, 0x24) == ACTOR_SOLID_ON_TOP)
        k86(a)->stood++;
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 4));
}

/*
 * @routine 84:97BB Kind86ChildAbove
 * WHAT IT DOES: Keeps its child $2B pixels above it.
 */
static void Kind86ChildAbove(Actor *a)
{
    Actor *c = child_of(a);

    if (c)
        c->y = with_pixels(c->y, (uint16_t)(pixels(a->y) - ABOVE));
}

/*
 * @routine 84:97EE Kind86OutOfView
 * WHAT IT DOES: True when it is more than $E0 across or $100 up or down
 * from the range point (g.range_x, g.range_y).
 */
static int Kind86OutOfView(const Actor *a)
{
    if ((uint16_t)(g.range_x - pixels(a->x) + REACH_X + view.dx) >= (uint16_t)(SPAN_X + 2 * view.dx))
        return 1;
    return (uint16_t)(g.range_y - pixels(a->y) + REACH_Y + view.dy) >= (uint16_t)(SPAN_Y + 2 * view.dy);
}

/* A frame of its cycle after the hold: its height kept and the stand
   check, as the frame begins. */
static void turn(Actor *a)
{
    Kind86HeightKeep(a);
    Kind86StandCheck(a);
}

/* The cycle's start: the hold. */
static int cycle(Actor *a)
{
    k86(a)->count = 0x10;
    Kind86HeightKeep(a);
    a->resume = HOLD;
    return 1;
}

/* `frames` frames of the cycle's step `resume`. */
static int step(Actor *a, uint16_t resume, uint8_t frames)
{
    k86(a)->count = frames;
    a->resume = resume;
    return 1;
}

/*
 * @routine 84:962D Kind86Start
 * WHAT IT DOES: Actor kind $86 (through 83:92FF Kind86Enter): look $0600,
 * anim_flags 2, flags $90, view box 8. The child (+$07 bit 2) shows pose
 * $0F and stays. The placed one makes its child (its own kind and look,
 * +$07 with bit 2, facing right) at spawn point 0, and from the next frame
 * shows pose $0E and keeps its starting height; then for good: a hold of
 * 16 frames; a drop ($20 frames, $14 with +$07 bits 0-1 set; gravity $28)
 * ending with sound $39; 2 frames, a pixel up, 2 frames, the pixel back;
 * $40 frames still; then sound $38 and the rise (speed entry $7B) until it
 * is above its starting height. Each frame after the hold begins with its
 * height kept and the stand check; the player standing on it rides with
 * it.
 * Its second entry (state $8C, which its wait routine sets once it is out
 * of view): 84:980F Kind86End (through 83:9308), ActorEndPlaced.
 * The original calls ActorSpawnPlace before ActorSpawn with no child in X
 * (X = $0C, the loop's jump index): its writes land in the direct page's
 * scratch $0018-$0019 and $001B-$001C, which hold nothing between
 * routines; ActorSpawn then places the child itself, so the call is not
 * kept. ActorSpawn takes the kind in A, there the low byte the stray call
 * left; the child's kind is written right after, so its own kind is
 * passed here. The ten bytes after the BRA at 84:968C (84:968E
 * Kind86Unused) are reached by nothing (the record's proof): not
 * converted.
 */
int Kind86Start(Actor *a)
{
    K86Own *k = k86(a);
    Actor *c;

    switch (a->resume) {
    case ACTOR_START:
        a->look = 0x0600;
        a->anim_flags = 0x02;
        a->flags = 0x90;
        a->view_box = 0x08;
        if (a->spawn_byte & 0x04) {
            AnimStart(a, 0x0F);
            a->resume = CHILD;
            return 1;
        }
        k->own_4f = 0;
        a->spawn_at = 0;
        c = ActorSpawn(a, a->kind);
        if (c) {
            c->kind = a->kind;
            c->spawn_byte = (uint8_t)((a->spawn_byte & 0xFB) | 0x04);
            c->facing = 0;
            c->flip = 0;
            c->look = a->look;
            k->pair = (uint8_t)(c - g.actors);
        }
        a->resume = SPAWNED;
        return 1;
    case CHILD:
        return 1;
    case SPAWNED:
        AnimStart(a, 0x0E);
        set16(k->home_y, pixels(a->y));
        return cycle(a);
    case HOLD:
        Kind86StandCheck(a);
        if (--k->count)
            return 1;
        k->left = (a->spawn_byte & 0x03) ? 0x14 : 0x20;
        ActorMotionReset(a, 0x28);
        a->resume = DROP;
        return 1;
    case DROP:
        turn(a);
        ActorFall(a);
        Kind86Carry(a);
        if (--k->left)
            return 1;
        SoundQueueAdd(0x39);
        return step(a, BOUNCE_1, 0x02);
    case BOUNCE_1:
        turn(a);
        Kind86Carry(a);
        if (--k->count)
            return 1;
        a->resume = BOUNCE_UP;
        return 1;
    case BOUNCE_UP:
        turn(a);
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 1));
        Kind86Carry(a);
        return step(a, BOUNCE_2, 0x02);
    case BOUNCE_2:
        turn(a);
        Kind86Carry(a);
        if (--k->count)
            return 1;
        a->resume = BOUNCE_DOWN;
        return 1;
    case BOUNCE_DOWN:
        turn(a);
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 1));
        Kind86Carry(a);
        k->count = 0x40;
        Kind86HeightKeep(a);
        a->resume = STILL;
        return 1;
    case STILL:
        turn(a);
        Kind86Carry(a);
        if (--k->count)
            return 1;
        SoundQueueAdd(0x38);
        ActorSpeedYSet(a, 0x7B);
        a->resume = RISE;
        return 1;
    case RISE:
        turn(a);
        ActorStepYBack(a);
        Kind86Carry(a);
        if (pixels(a->y) >= get16(k->home_y))
            return 1;
        return cycle(a);
    default:                            /* ACTOR_START_SECOND: 84:980F Kind86End */
        return ActorEndPlaced(a);
    }
}

/*
 * @routine 83:9303 ActorKind86Wait
 * WHAT IT DOES: Kind $86's wait routine (each frame it waits, flags bit
 * 7), 84:9766 Kind86WaitWork: the child does nothing; the placed one may
 * hold the player (at offset 6), hurts him in its zone $5C (which ends its
 * drop), animates, keeps its child above it, and once out of view frees
 * the child and sets its own state bits $8C (its second entry from its
 * next turn).
 */
int Kind86Wait(Actor *a)
{
    Actor *c;

    if (a->spawn_byte & 0x04)
        return ACTOR_GOES_ON;
    k86(a)->hold_at = 0x06;
    ActorHoldPlayer(a);
    if (ActorZoneHurtsPlayer(a, 0x5C))
        k86(a)->left = 0x01;
    AnimStep(a);
    Kind86ChildAbove(a);
    if (!Kind86OutOfView(a))
        return ACTOR_GOES_ON;
    c = child_of(a);
    if (c)
        ActorFreePlaced(c);
    a->state |= 0x8C;
    return ACTOR_GOES_ON;
}
