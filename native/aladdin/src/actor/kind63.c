/*
 * kind63.c: actor kind $63, which watches for Aladdin to come within reach
 * below it and throws at him (two children of kind $A2 each round), its
 * work each frame it waits, its knock-away when hit (its second state) and
 * its answer to a shot (its third).
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_42];
    uint8_t out;                /* +$42: its throws still flying (spawn_throw counts them) */
    uint8_t hit;                /* +$43: nonzero while it shows being hit: no shot test then */
    uint8_t aim;                /* +$44: how far below it he is: 2 near, 1 farther, 0 farthest */
    uint8_t unused45;
    uint8_t out_hit;            /* +$46: its throws flying as a shot found it (shot_hit_face) */
    uint8_t gone;               /* +$47: counted up each frame it is out of range, and when knocked */
    uint8_t shot_x[2];          /* +$48-$49: where the shot that hit it was (pixels): its pair starts there */
    uint8_t unused4c[OWN_4A - OWN_4C];
    uint8_t shot_y[2];          /* +$4A-$4B */
} Kind63Own;

_Static_assert(sizeof(Kind63Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind63Own *own(Actor *a) { return (Kind63Own *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

/*
 * @routine 84:8145 ActorSpawnA2
 * WHAT IT DOES: Starts a child of kind $A2 (a throw) at spawn point $82,
 * telling it its parent (its +$4C word: the spawner's index in g.actors)
 * and its +$07: the spawner's +$07 less 3 plus `add` when that +$07 is 3
 * or more, else the spawner's aim (+$44); the spawner's +$42 counts it.
 * Kind $63 is its only caller.
 */
static void spawn_throw(Actor *a, uint8_t add)
{
    Actor *c;
    uint16_t me = (uint16_t)(a - g.actors);

    a->spawn_at = 0x82;
    c = ActorSpawn(a, 0xA2);
    if (!c)
        return;
    c->spawn_byte = a->spawn_byte >= 3 ? (uint8_t)(a->spawn_byte - 3 + add) : own(a)->aim;
    c->own.bytes[OWN_4C] = (uint8_t)me;
    c->own.bytes[OWN_4D] = (uint8_t)(me >> 8);
    own(a)->out++;
}

/*
 * @routine 84:8173 ActorSpawnPairA3
 * WHAT IT DOES: Starts two children of kind $A3, their +$07 0 and 1, each
 * at the place the shot that hit it was (+$48, +$4A); while it runs its
 * +$43 is set. A spawn with no record free ends it. Kind $63 is its only
 * caller.
 */
static void spawn_pair(Actor *a)
{
    uint8_t which;

    own(a)->hit++;
    for (which = 0; which < 2; which++) {
        Actor *c;

        a->spawn_at = 0;
        c = ActorSpawn(a, 0xA3);
        if (!c)
            break;
        c->spawn_byte = which;
        c->x = with_pixels(c->x, word(own(a)->shot_x));
        c->y = with_pixels(c->y, word(own(a)->shot_y));
    }
    own(a)->hit = 0;
}

/*
 * @routine 84:821A ActorShotHitFace
 * WHAT IT DOES: When one of the second pool's shots touches the actor: it
 * faces Aladdin, takes state $CC (hit), keeps its throws flying (+$42) in
 * +$46 (with none, the SHOT is put into state $8C: ActorStateSet8C is given
 * the record the touch test found, not this one), and keeps the shot's
 * place in +$48 and +$4A. Kind $63's wait is its only caller.
 */
static void shot_hit_face(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return;
    a->facing = a->flip = pixels(a->x) >= pixels(g.actors[0].x);
    a->state = 0xCC;
    own(a)->out_hit = own(a)->out;
    if (!own(a)->out)
        ActorStateSet8C(shot);
    set_word(own(a)->shot_x, pixels(shot->x));
    set_word(own(a)->shot_y, pixels(shot->y));
}

enum {
    WATCH_SET = ACTOR_RESUME_OWN,   /* after its first 16 frames of watching */
    WATCH,                          /* each frame of the watch */
    THROW_POSE,                     /* after 8 frames turned to him */
    THROW,                          /* after 16 frames of animation 4 */
    THROWN,                         /* after 8 frames: the second throw */
    AGAIN_POSE,                     /* after 40 frames of animation 1 */
    AGAIN,                          /* after 16 frames of animation 5 */
    AGAIN_THROWN,                   /* after 8 frames */
    WAIT_TWO,                       /* each frame while two fly */
    WAIT_ONE,                       /* each frame while one flies */
    PAIR_THROWN,                    /* after its pair's 24 frames */
    HIT_SHOWN,                      /* after its hit's 24 frames */
    HIT_OVER,                       /* a frame after */
    KNOCKED,                        /* knocked away (ActorKnockedAway, called again until it ends it) */
};

/*
 * @routine 84:80AA Kind63Watch
 * WHAT IT DOES: Kind $63 watches (animation 0) until Aladdin is within box
 * $30 and within reach below it, then turns toward him, picks how far to
 * throw, and throws once (animation 4, a child of kind $A2).
 */
static int watch(Actor *a)
{
    AnimStart(a, 0);
    a->resume = WATCH_SET;
    return 0x10;
}

static int watch_frame(Actor *a)
{
    uint8_t at = (uint8_t)(a->spawn_byte << 1);
    uint16_t below;
    uint8_t side, low;

    if (!ActorNearPlayer(a, 0x30))
        return 1;
    /* its x against his: 1 when he is at or left of it */
    side = pixels(a->x) >= pixels(g.actors[0].x);
    below = (uint16_t)(pixels(a->y) + k63_reach_y(at));
    if (below < pixels(g.actors[0].y))
        return 1;
    low = (uint8_t)(below - pixels(g.actors[0].y));
    own(a)->aim = low < k63_aim_near(at) ? 2 : low < k63_aim_mid(at) ? 1 : 0;
    a->facing = a->flip = side;
    a->resume = THROW_POSE;
    return 8;
}

/*
 * @routine 84:810B Kind63ThrowAgain
 * WHAT IT DOES: Kind $63's second throw: a pause (animation 1), then
 * animation 5 and a child of kind $A2.
 */
static int throw_again(Actor *a)
{
    AnimStart(a, 1);
    a->resume = AGAIN_POSE;
    return 0x28;
}

/*
 * @routine 84:8128 Kind63WaitThrows
 * WHAT IT DOES: Kind $63 waits for its throws to be gone (animation 2
 * while two fly, 1 while one does), then watches again.
 */
static int wait_throws(Actor *a)
{
    AnimStart(a, 2);
    a->resume = WAIT_TWO;
    return 1;
}

/*
 * @routine 84:8080 Kind63Start
 * WHAT IT DOES: Actor kind $63's start: shown facing right with one hit
 * of health and nothing thrown yet, then on to watch for Aladdin.
 */
static int start(Actor *a)
{
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    a->redraw = 0xFF;
    a->probes = k63_probes();
    a->own_pictures = k63_pictures();
    own(a)->out = 0;
    own(a)->hit = 0;
    own(a)->gone = 0;
    a->own.bytes[OWN_11] = 1;
    a->facing = 0;
    a->flip = 0;
    return watch(a);
}

/*
 * @routine 84:81CD Kind63ShotHit
 * WHAT IT DOES: Kind $63 hit by a shot (state $CC): with nothing thrown
 * when it was hit, it throws a pair (animation 6) and watches again; else
 * it shows the hit (animation 3) and goes on waiting for its throws, or
 * throws again when one is left.
 */
static int shot_hit(Actor *a)
{
    if (!own(a)->out_hit) {
        AnimStart(a, 6);
        spawn_pair(a);
        a->resume = PAIR_THROWN;
        return 0x18;
    }
    own(a)->hit++;
    AnimStart(a, 3);
    a->resume = HIT_SHOWN;
    return 0x18;
}

/*
 * @routine 84:81BE Kind63Knocked
 * WHAT IT DOES: Actor kind $63 in its second state (hit): its +$47 counted
 * up, its sprite priority cleared, and it is knocked away with animation
 * 8, sound $0F and motion preset 3.
 */
static int knocked(Actor *a)
{
    own(a)->gone++;
    ActorPriorityClear(a);
    a->resume = KNOCKED;
    return ActorKnockedAway(a, 8, 0x0F, 3);
}

int Kind63Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return knocked(a);
    case ACTOR_START_THIRD:
        return shot_hit(a);
    case WATCH_SET:
        a->resume = WATCH;
        return 1;
    case WATCH:
        return watch_frame(a);
    case THROW_POSE:
        AnimStart(a, 4);
        a->resume = THROW;
        return 0x10;
    case THROW:
        spawn_throw(a, 1);
        a->resume = THROWN;
        return 8;
    case THROWN:
        return throw_again(a);
    case AGAIN_POSE:
        AnimStart(a, 5);
        a->resume = AGAIN;
        return 0x10;
    case AGAIN:
        spawn_throw(a, 0);
        a->resume = AGAIN_THROWN;
        return 8;
    case AGAIN_THROWN:
        return wait_throws(a);
    case WAIT_TWO:
        if (own(a)->out >= 2)
            return 1;
        AnimStart(a, 1);
        a->resume = WAIT_ONE;
        return 1;
    case WAIT_ONE:
        if (own(a)->out)
            return 1;
        return watch(a);
    case PAIR_THROWN:
        return watch(a);
    case HIT_SHOWN:
        own(a)->hit = 0;
        a->resume = HIT_OVER;
        return 1;
    case HIT_OVER:
        if ((uint8_t)(own(a)->out - 1))
            return wait_throws(a);
        return throw_again(a);
    case KNOCKED:
        return ActorKnockedAway(a, 8, 0x0F, 3);
    default:
        return kind_lost(a, "Kind63Code");
    }
}

/*
 * @routine 83:924D ActorKind63Wait
 * @routine 84:81FC Kind63Work
 * WHAT IT DOES: Kind $63's wait routine (the loop's stub JSLs the work):
 * unless already hit, a shot turns it to face Aladdin and hits it; Aladdin
 * is hurt by it or stomps it; it animates and shows its own picture; and
 * out of range by box $28 it is freed, its +$47 counted up.
 */
int Kind63Wait(Actor *a)
{
    if (!own(a)->hit)
        shot_hit_face(a);
    (void)ActorStompOrHurt(a);
    AnimStep(a);
    ActorPictureSend(a);
    if (ActorFreeOutOfRange(a, 0x28)) {
        own(a)->gone++;
        return ACTOR_ENDED;
    }
    return ACTOR_GOES_ON;
}
