/*
 * kind41.c: actor kind $41. With +$07 bits 1-2 clear it is the walker: it
 * rises out of the ground when Aladdin is in reach, starts the layer 1
 * wobble, hops twice and walks, hopping again where its ground ends. With
 * them set it is a maker of walkers: one each time Aladdin is in reach
 * (bits 3-4 clear), or one every so often at its own place (the timed
 * maker, which gives each walker its place as limits). Its wait routine is
 * Kind41Wait (kind_waits_rest.c). Its second entry ends it.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "map/map.h"

#define K41_HOP_SOUND 0x1B      /* the sound of each hop */
#define K41_REACH_SPAWN 0x2E    /* the reach maker's spawn point */
#define K41_BOX 5               /* PlayerInReach's entry for the reach maker */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t unused40;
    uint8_t pause;              /* +$41: the maker: frames between its walkers */
    uint8_t active;             /* +$42: nonzero: the walker (Kind41Wait: it turns shots back, Aladdin can stand on it) */
    uint8_t frames;             /* +$43: Kind41Wait: frames before he can stand on it again */
} Kind41Own;

_Static_assert(sizeof(Kind41Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind41Own *own(Actor *a) { return (Kind41Own *)a->own.bytes; }

static int returned(int r) { return r == ACTOR_RETURNS || r == ACTOR_RETURNS_C; }

enum {
    REACH = ACTOR_RESUME_OWN,   /* the reach maker: each frame until Aladdin is in reach */
    REACH_SPAWN,                /* ActorSpawnLookWait, called again until it returns */
    TIMED_SPAWN,                /* the timed maker: the same, each turn */
    PAUSED,                     /* a maker: after its pause, one more frame before it goes on */
    RISE,                       /* the walker: ActorRiseWhenInReach, box 0 */
    HOP_FIRST,                  /* ActorHopFromTable 0 */
    HOP_SECOND,                 /* ActorHopFromTable 5 */
    WALK,                       /* each frame of its walk */
    HOP_BACK,                   /* ActorHopFromTable $1E, where the ground ends */
};

/* The pause a maker waits after each walker: frames, 0 being 256. */
static int pause_frames(Actor *a)
{
    return own(a)->pause ? own(a)->pause : 256;
}

static int hop_second(Actor *a);

/* The walker's first hop: after the rise's sound. */
static int hop_first(Actor *a)
{
    int r = ActorHopFromTable(a, 0);

    if (!returned(r))
        return r;
    SoundQueueAdd(K41_HOP_SOUND);
    a->resume = HOP_SECOND;
    return hop_second(a);
}

static int hop_second(Actor *a)
{
    int r = ActorHopFromTable(a, 5);

    if (!returned(r))
        return r;
    SoundQueueAdd(K41_HOP_SOUND);
    AnimStart(a, 1);
    ActorSpeedYSet(a, 0x1E);
    a->resume = WALK;
    return 1;
}

static int rise(Actor *a)
{
    int r = ActorRiseWhenInReach(a, 0);

    if (!returned(r))
        return r;
    TaskCreate(4, 4, TASK_ENTRY_BG1_WOBBLE);
    SoundQueueAdd(K41_HOP_SOUND);
    a->resume = HOP_FIRST;
    return hop_first(a);
}

static int hop_back(Actor *a)
{
    int r = ActorHopFromTable(a, 0x1E);

    if (!returned(r))
        return r;
    SoundQueueAdd(K41_HOP_SOUND);
    a->resume = WALK;
    return 1;
}

static int walk(Actor *a)
{
    ActorMoveX(a);
    (void)ActorOutOfWallProbe4(a);
    if (ActorSnapToGround(a, NULL))
        return 1;
    a->resume = HOP_BACK;
    return hop_back(a);
}

/* The reach maker's spawn: one more of its kind, then its pause. */
static int reach_spawn(Actor *a)
{
    int r = ActorSpawnLookWait(a);

    if (!returned(r))
        return r;
    a->resume = PAUSED;
    return pause_frames(a);
}

/* The timed maker's spawn: the walker it took gets its place as its limits
   (its +$48 the x, its +$4A the y), then its pause. */
static int timed_spawn(Actor *a)
{
    int r = ActorSpawnLookWait(a);
    int at = g.free_pool_top + 1;

    if (!returned(r))
        return r;
    if (at < ACTOR_POOL_SIZE) {
        Actor *child = g.free_pool[at];

        child->own.bytes[OWN_48] = (uint8_t)pixels(a->x);
        child->own.bytes[OWN_49] = (uint8_t)(pixels(a->x) >> 8);
        child->own.bytes[OWN_4A] = (uint8_t)pixels(a->y);
        child->own.bytes[OWN_4B] = (uint8_t)(pixels(a->y) >> 8);
    }
    a->resume = PAUSED;
    return pause_frames(a);
}

/*
 * @routine 83:B201 Kind41Start
 * WHAT IT DOES: Actor kind $41. The walker (+$07 bits 1-2 clear): faces by
 * +$07 bit 0, rises out of the ground when Aladdin is in reach, starts the
 * layer 1 wobble (Bg1WobbleTask), hops twice with sound $1B, then walks,
 * and where it finds no ground under it hops again. The maker (bits 1-2
 * set): with bits 3-4 clear, each time Aladdin is in reach it makes one
 * more of its kind (+$07 AND $19, so a walker), then pauses; with them set
 * it makes one every pause at its spawn point and gives it its own x and y
 * as that walker's limits.
 * Not converted: the original's cartridge RAM read for the walker's first
 * animation ($70:1200, written and read back: on the retail board it
 * comes to 0, so animation 0).
 */
static int start(Actor *a)
{
    Kind41Own *o = own(a);
    uint8_t byte07 = a->spawn_byte;

    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    a->probes = k41_probes();
    o->active = 0;
    o->frames = 0;
    if (!(byte07 & 0x06)) {
        a->facing = a->flip = (uint8_t)(byte07 & 0x01);
        o->active++;
        AnimStart(a, 0);
        a->resume = RISE;
        return rise(a);
    }
    a->facing = a->flip = 0;
    o->pause = k41_pause((uint8_t)((byte07 >> 1) & 0x03));
    a->spawn_byte = (uint8_t)(byte07 & 0x19);
    a->anim_flags &= (uint8_t)~0x08;
    a->own.bytes[OWN_4D] = a->kind;
    if (!(byte07 & 0x18)) {
        a->spawn_at = K41_REACH_SPAWN;
        a->resume = REACH;
        return 1;
    }
    a->spawn_at = k41_spawn_at((uint8_t)((byte07 & 0x18) >> 3));
    a->resume = TIMED_SPAWN;
    return 1;
}

int Kind41Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case REACH:
        if (!PlayerInReach(a, K41_BOX))
            return 1;
        a->resume = REACH_SPAWN;
        return reach_spawn(a);
    case REACH_SPAWN:
        return reach_spawn(a);
    case TIMED_SPAWN:
        return timed_spawn(a);
    case PAUSED:
        a->resume = a->spawn_byte & 0x18 ? TIMED_SPAWN : REACH;
        return 1;
    case RISE:
        return rise(a);
    case HOP_FIRST:
        return hop_first(a);
    case HOP_SECOND:
        return hop_second(a);
    case WALK:
        return walk(a);
    case HOP_BACK:
        return hop_back(a);
    default:
        return kind_lost(a, "Kind41Code");
    }
}
