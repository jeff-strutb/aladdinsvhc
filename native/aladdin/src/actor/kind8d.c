/*
 * kind8d.c: actor kind $8D, which flies in from the view's left, circles
 * Aladdin twice, flies off and ends; its wait routine (84:9B4D
 * SenderWait8E) makes a kind $8E (a pose of it) every eight frames; and
 * kind $8E, by its +$07 a pose or a burst of copies of itself. Kind $8D's
 * second entry (84:9B43) ends it off the screen.
 */
#include "actor/kinds_c.h"
#include "video/view.h"
#include "data/kinds_c_tables.h"

/* its own bytes (kind $8D keeps the turns, kind $8E its frames and copies) */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t count;              /* +$40: kind $8D: circles left, then frames left of its flight off; kind $8E: frames left */
    uint8_t copies;             /* +$41: kind $8E: copies left to make (its spawner sets it) */
} Kind8DOwn;

_Static_assert(sizeof(Kind8DOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind8DOwn *own(Actor *a) { return (Kind8DOwn *)a->own.bytes; }

enum {
    IN = ACTOR_RESUME_OWN,      /* kind $8D: each frame flying in */
    CIRCLE,                     /* each frame circling him */
    OFF,                        /* each frame flying off */
    POSE,                       /* kind $8E: each frame of its pose */
    BURST,                      /* each frame of a burst's two */
};

/*
 * @routine 84:9B43 Kind8DGone
 * WHAT IT DOES: Kind $8D's end: off the screen for good (its x and y high
 * bytes $FF), its record freed.
 */
static int gone(Actor *a)
{
    a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) | 0xFF00));
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) | 0xFF00));
    return ActorEndPlaced(a);
}

/*
 * @routine 84:99ED Kind8DCircle
 * WHAT IT DOES: Actor kind $8D: starts 16 pixels left of the view and 256
 * below its top (32 below the view's bottom), flies on direction $1B until near Aladdin (box $2C), then
 * circles him twice at radius $48 (its flip toward him), flies off on
 * direction $1D for 80 frames, and ends off the screen.
 */
static int start8d(Actor *a)
{
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    a->view_box = 0x18;
    a->own_pictures = k8d_pictures();
    a->x = with_pixels(a->x, (uint16_t)(pixels(g.layers[0].scroll_x) - 0x10));
    a->y = with_pixels(a->y, (uint16_t)(pixels(g.layers[0].scroll_y) + 0x100 + 2 * view.dy));
    AnimStart(a, 0);
    a->facing = 0x1B;
    a->flip = 0;
    a->resume = IN;
    return 1;
}

static int in8d(Actor *a)
{
    ActorMoveByVector(a, 0x1E);
    if (!ActorNearPlayer(a, 0x2C))
        return 1;
    a->facing = 0x46;
    own(a)->count = 2;
    a->resume = CIRCLE;
    return 1;
}

static int circle8d(Actor *a)
{
    Actor *player = &g.actors[0];

    a->x = with_pixels(a->x, pixels(player->x));
    a->y = with_pixels(a->y, pixels(player->y));
    a->facing = (uint8_t)((a->facing + 1) & 0x7F);
    ActorMoveByVectorTimes(a, 0x10, 0x48);
    a->flip = ActorSideOfPlayer(a);
    if (a->facing != 0x48 || --own(a)->count)
        return 1;
    a->flip = 0;
    a->facing = 0x1D;
    own(a)->count = 0x50;
    a->resume = OFF;
    return 1;
}

static int off8d(Actor *a)
{
    ActorMoveByVector(a, 0x1E);
    if (--own(a)->count)
        return 1;
    g.scroll_y_held = 0;
    return gone(a);
}

int Kind8DCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start8d(a);
    case ACTOR_START_SECOND:
        return gone(a);
    case IN:
        return in8d(a);
    case CIRCLE:
        return circle8d(a);
    case OFF:
        return off8d(a);
    default:
        return kind_lost(a, "Kind8DCode");
    }
}

/*
 * @routine 84:9B4D SenderWait8E
 * @routine 83:9348 ActorKind8DWait
 * WHAT IT DOES: Kind $8D's wait routine: its animation on and its picture
 * sent; when the low three bits of g.sprite_flicker are 0, a child of kind
 * $8E (+$07 1) at its spawn point $80.
 */
int Kind8DWait(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    if (!(g.sprite_flicker & 0x07)) {
        a->spawn_at = 0x80;
        a->spawn_byte = 1;
        (void)ActorCreateChild8(a, 0x8E);
    }
    return ACTOR_GOES_ON;
}

/*
 * @routine 84:9B6B Kind8EStart
 * WHAT IT DOES: Actor kind $8E, by its +$07: 0 and 1 a pose (0 or 1) for 32
 * frames, then it ends; 2 or more a burst: its +$41 times a copy of itself
 * (+$07 0) made and the actor moved off by vector set $20 for two frames,
 * then it ends.
 */
static int start8e(Actor *a)
{
    if (a->spawn_byte < 2) {
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        AnimStart(a, a->spawn_byte);
        own(a)->count = 0x20;
        a->resume = POSE;
        return 1;
    }
    a->spawn_byte = 0;
    a->spawn_at = 0;
    (void)ActorCreateChild8(a, a->kind);
    own(a)->count = 2;
    a->resume = BURST;
    return 1;
}

int Kind8ECode(Actor *a)
{
    Kind8DOwn *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        return start8e(a);
    case POSE:
        AnimStep(a);
        if (--o->count)
            return 1;
        return ActorEndPlaced(a);
    case BURST:
        ActorMoveByVector(a, 0x20);
        if (--o->count)
            return 1;
        if (!--o->copies)
            return ActorEndPlaced(a);
        (void)ActorCreateChild8(a, a->kind);
        o->count = 2;
        return 1;
    default:
        return kind_lost(a, "Kind8ECode");
    }
}
