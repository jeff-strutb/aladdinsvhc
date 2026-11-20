/*
 * kind4f.c: actor kind $4F, a thing that opens once when struck: unless
 * the flag its +$07 names in g.opened_flags is already set (then it shows
 * open), it waits to be struck, then sounds, opens and lets out its
 * children. Its wait routine (ActorWaitTest42Anim2, here) marks it
 * struck when Aladdin bounces on it, and makes it solid for him.
 *
 * Its second entry in the original's table (83:D108) is the RTS that ends
 * that wait routine, which the loop's jump would return through into its
 * own saved D: no code sets a kind $4F's state past $7F, and it has none.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a pause */
    uint8_t bit;                /* +$41: its bit in g.opened_flags */
    uint8_t struck;             /* +$42: set when it is struck (by its wait routine), or at once when already open */
} Kind4FOwn;

_Static_assert(sizeof(Kind4FOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind4FOwn *own(Actor *a) { return (Kind4FOwn *)a->own.bytes; }

enum {
    CLOSED = ACTOR_RESUME_OWN,  /* each frame until it is struck */
    OPENING,                    /* each frame of the 5 before it opens */
    OPENED,                     /* each frame of the 5 after the ring */
    STAY,
};

/*
 * @routine 83:D01A Kind4FStart
 * WHAT IT DOES: Actor kind $4F: shown closed (pose 9) until struck, then
 * sound $12, 5 frames, open (pose $0A), a ring of children, 5 frames, its
 * variant's child, and it stays open; when the flag its +$07 names is
 * already set in g.opened_flags it is shown open at once.
 */
static int start(Actor *a)
{
    Kind4FOwn *o = own(a);

    a->anim_flags |= 0x03;
    a->flags |= 0x90;
    a->look |= 0x0200;
    o->struck = 0;
    o->bit = k4f_opened_bit((uint8_t)(a->spawn_byte & 0x03));
    if (o->bit & g.opened_flags) {
        o->struck++;
        AnimStart(a, 0x0A);
        a->resume = STAY;
        return 1;
    }
    AnimStart(a, 9);
    a->resume = CLOSED;
    return 1;
}

int Kind4FCode(Actor *a)
{
    Kind4FOwn *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case CLOSED:
        if (!o->struck)
            return 1;
        o->frames = 0x05;
        SoundQueueAdd(0x12);
        a->resume = OPENING;
        return 1;
    case OPENING:
        if (--o->frames)
            return 1;
        AnimStart(a, 0x0A);
        ActorSpawnRing(a);
        o->frames = 0x05;
        a->resume = OPENED;
        return 1;
    case OPENED:
        if (--o->frames)
            return 1;
        ActorVariantChildStep(a);
        a->resume = STAY;
        return 1;
    case STAY:
        return 1;
    default:
        return kind_lost(a, "Kind4FCode");
    }
}

/*
 * @routine 83:D0EF ActorWaitTest42Anim2
 * WHAT IT DOES: Kind $4F's wait routine: each frame it waits, until it is
 * struck, Aladdin bouncing on it strikes it (+$42 up); it is solid for him
 * by box $20; its animation on; it ends once out of range by box $10.
 */
int ActorWaitTest42Anim2(Actor *a)
{
    if (!own(a)->struck && ActorBouncesPlayer(a))
        own(a)->struck++;
    (void)ActorSolidForPlayer(a, 0x20);
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x10);
}
