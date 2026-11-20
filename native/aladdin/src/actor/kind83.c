/*
 * kind83.c: actor kind $83, a thing that flies up out of the carpet
 * levels' scenery to a height and leaves a child of kind $5E there. Its
 * wait routine (83:92DC's stub, 84:95D4 ActorWaitGrabsPlayer) plays its
 * animation and puts Aladdin into state $8C when he meets its box. Its
 * second entry in the original's table is that same wait routine: no code
 * sets a kind $83's state past $7F, and it has none.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "player/player.h"

#define KIND_5E 0x5E

/* its own bytes */
typedef struct {
    uint8_t before[OWN_42];
    uint8_t no_child;           /* +$42: nonzero (the late variant): no child at the end */
    uint8_t unused43[OWN_45 - OWN_43];
    uint8_t top_speed;          /* +$45: the most its vertical speed's fraction may reach */
    uint8_t unused46[OWN_4A - OWN_46];
    uint8_t line;               /* +$4A: the y (low byte) it goes on until: $C8, or $FF for the late variant */
} Kind83Own;

_Static_assert(sizeof(Kind83Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind83Own *own(Actor *a) { return (Kind83Own *)a->own.bytes; }

enum {
    LATE_WAIT = ACTOR_RESUME_OWN,   /* the late one: each frame until Aladdin is past it */
    SHOW,                           /* after its delay: it shows */
    FLY,                            /* each frame of its flight */
    FALL,                           /* each frame of its fall to its line */
    CHILD,                          /* each frame until its child is made */
};

/* Facing right, its top speed and its motion preset by +$07; flying. */
static int fly_set(Actor *a)
{
    a->facing = a->flip = 0;
    own(a)->top_speed = k83_top_speed(a->spawn_byte);
    ActorMotionPreset(a, k83_preset(a->spawn_byte));
    a->resume = FLY;
    return 1;
}

/* At its line: its child, unless it is the late one's, and its end. */
static int there(Actor *a)
{
    if (own(a)->no_child)
        return ActorEndPlaced(a);
    a->resume = CHILD;
    return 1;
}

/*
 * @routine 84:9538 Kind83Start
 * WHAT IT DOES: Actor kind $83: by its +$07, hidden for a while, then it
 * flies off by its motion preset, its vertical speed held to its top
 * speed, until its y reaches $C8; it falls instead once the view stops
 * following ($1A10 set); then it leaves a child of kind $5E and ends. With
 * +$07 bit 4 set (late) it waits until Aladdin is past it, flies until y
 * $FF, and leaves no child.
 */
static int start(Actor *a)
{
    uint8_t delay;

    a->flags |= 0x90;
    a->probes = k83_probes();
    AnimStart(a, 0);
    own(a)->no_child = 0;
    if (a->spawn_byte & 0x10) {
        a->spawn_byte &= 0x0F;
        own(a)->no_child++;
        a->resume = LATE_WAIT;
        return 1;
    }
    a->anim_flags &= (uint8_t)~0x08;
    delay = k83_delay(a->spawn_byte);
    a->resume = SHOW;
    return delay ? delay : 256;
}

static int flying(Actor *a)
{
    Kind83Own *o = own(a);

    ActorFly(a);
    if ((uint8_t)pixels(a->y) >= o->line)
        return there(a);
    if (g.scroll_x_held) {
        a->accel_y = 0x08;
        a->resume = FALL;
        return 1;
    }
    if (o->top_speed < (uint8_t)a->speed_y) {
        a->speed_y = (a->speed_y & ~0xFF) | o->top_speed;
        a->accel_y = 0;
    }
    return 1;
}

int Kind83Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case LATE_WAIT:
        if (pixels(a->x) >= pixels(g.actors[0].x))
            return 1;
        own(a)->line = 0xFF;
        return fly_set(a);
    case SHOW:
        a->anim_flags |= 0x08;
        own(a)->line = 0xC8;
        return fly_set(a);
    case FLY:
        return flying(a);
    case FALL:
        ActorFall(a);
        if ((uint8_t)pixels(a->y) < own(a)->line)
            return 1;
        return there(a);
    case CHILD:
        a->spawn_at = 0;
        if (ActorCreateChild8(a, KIND_5E))
            return ActorEndPlaced(a);
        return 1;
    default:
        return kind_lost(a, "Kind83Code");
    }
}

/*
 * @routine 84:95D4 ActorWaitGrabsPlayer
 * @routine 83:92DC ActorKind83Wait
 * WHAT IT DOES: Kind $83's wait routine: the actor's animation, and when
 * Aladdin, free to be caught, meets its box, he goes into state $8C
 * facing it.
 */
int Kind83Wait(Actor *a)
{
    AnimStep(a);
    if (ActorHitsTestBoxIfPlayerFree(a))
        (void)PlayerEnter8CFacing(a);
    return ACTOR_GOES_ON;
}
