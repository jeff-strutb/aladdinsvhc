/*
 * kind3e.c: actor kind $3E, a maker: while it cannot be hit it makes an
 * actor of look $31 next to it every 128 frames, as long as fewer than
 * three actors hold its tile slot. Its wait routine is Kind3EWait
 * (kind_waits_rest.c). Its second entry ends it.
 */
#include "actor/kinds_c.h"

#define K3E_MADE_LOOK 0x31      /* the look of what it makes */
#define K3E_SLOT_FULL 0x03      /* holders of its palette slot at which it makes none */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t left;               /* +$40: frames left of its pause after a spawn */
    uint8_t pause;              /* +$41: its pause's length (128 frames) */
} Kind3EOwn;

_Static_assert(sizeof(Kind3EOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind3EOwn *own(Actor *a) { return (Kind3EOwn *)a->own.bytes; }

static int returned(int r) { return r == ACTOR_RETURNS || r == ACTOR_RETURNS_C; }

enum {
    FRAME = ACTOR_RESUME_OWN,   /* each frame */
    SPAWN,                      /* ActorSpawnLookWait, called again until it returns */
    PAUSE,                      /* each frame of its pause after a spawn */
};

/*
 * @routine 83:B154 Kind3EStart
 * WHAT IT DOES: Actor kind $3E: each frame while it cannot be hit (flags
 * bit 6 clear) and fewer than three actors hold its palette slot, it makes
 * an actor of look $31 next to it, then pauses 128 frames.
 */
static int start(Actor *a)
{
    a->anim_flags &= (uint8_t)~0x08;
    a->flags |= 0x90;
    a->facing = a->flip = 0;
    own(a)->pause = 0x80;
    a->resume = FRAME;
    return 1;
}

static int spawn(Actor *a)
{
    int r = ActorSpawnLookWait(a);

    if (!returned(r))
        return r;
    a->resume = PAUSE;
    return 1;
}

static int frame(Actor *a)
{
    Kind3EOwn *o = own(a);

    o->left = o->pause;
    if (a->flags & 0x40) {
        o->pause = 0x80;
        return 1;
    }
    if (a->tile_slot && g.palette_slot_users[a->tile_slot - 1] >= K3E_SLOT_FULL)
        return 1;
    a->spawn_at = 0;
    a->own.bytes[OWN_4D] = K3E_MADE_LOOK;
    a->resume = SPAWN;
    return spawn(a);
}

int Kind3ECode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case FRAME:
        return frame(a);
    case SPAWN:
        return spawn(a);
    case PAUSE:
        if (--own(a)->left)
            return 1;
        a->resume = FRAME;
        return 1;
    default:
        return kind_lost(a, "Kind3ECode");
    }
}
