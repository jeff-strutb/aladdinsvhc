/*
 * kind2b.c: actor kind $2B: a walker along a floor, a wall or a ceiling
 * that turns at the ends of its leg, drops onto Aladdin from a wall or a
 * ceiling, or (+$07 bit 4) shakes and falls when he passes below, then
 * keeps by its partner; and its downed state. Its wait routine is
 * Kind2BWait (kind_waits_rest.c).
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "map/map.h"
#include "system/system.h"

/* its own bytes, as the walker keeps them */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t dir;                /* +$40: its direction along its surface, 0 or 1 */
    uint8_t leg[2];             /* +$41-$42: steps left of its leg */
    uint8_t leg2[2];            /* +$43-$44: twice its leg */
    uint8_t leg_kept[2];        /* +$45-$46: its leg, for the floor after a drop */
    uint8_t surface;            /* +$47: 0 ceiling, 1 floor, 2-3 walls ($07 bits 0-2 / 2) */
    uint8_t from_x[2];          /* +$48-$49: the x its range across is judged from */
    uint8_t range[2];           /* +$4C-$4D: its range across */
    uint8_t stays;              /* +$4E: nonzero: never ended out of range */
    uint8_t unused4f;
    uint8_t range2[2];          /* +$4A-$4B: twice its range */
} Kind2BOwn;

/* the same bytes as the faller (+$07 bit 4) keeps them */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of its way */
    uint8_t way_frames;         /* +$41: frames of each way */
    uint8_t kept_x[2];          /* +$42-$43: its x before a step by its partner */
    uint8_t kept_y[2];          /* +$44-$45: its y */
    uint8_t shakes;             /* +$46 */
    uint8_t surface;            /* +$47 */
    uint8_t unused48;
    uint8_t partner_low;        /* +$49: its partner's index in g.actors (low byte; its maker sets it) */
    uint8_t unused4c[OWN_4E - OWN_4C];
    uint8_t stays;              /* +$4E */
    uint8_t unused4f;
    uint8_t partner_high;       /* +$4A */
    uint8_t landed;             /* +$4B: set by its partner: its fall or jump is over (its wait clears it) */
} Kind2BFallerOwn;

_Static_assert(sizeof(Kind2BOwn) <= OWN_BYTES && sizeof(Kind2BFallerOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind2BOwn *own(Actor *a) { return (Kind2BOwn *)a->own.bytes; }
static Kind2BFallerOwn *faller(Actor *a) { return (Kind2BFallerOwn *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

enum {
    TURNED = ACTOR_RESUME_OWN,  /* after its turn's 20 frames */
    WALK,                       /* each frame of its walk */
    DROP,                       /* each frame of its drop */
    DROPPED,                    /* after its 32 frames on the floor */
    WANDER,                     /* the faller: each frame to and fro */
    WANDER_TURNED,              /* after its turn's 16 frames */
    SHAKE,                      /* after each shake's 6 frames */
    FALL,                       /* each frame of its fall */
    BY_PARTNER,                 /* each frame by its partner */
    JUMP,                       /* each frame of a jump */
    DOWNED,                     /* its second entry: each frame of its fall away */
};

/* surface * 2 + direction: the way it goes, by which the tables are read */
static uint8_t way(Actor *a) { return (uint8_t)(own(a)->surface * 2 + own(a)->dir); }

/* A step and the next frame: by its direction's vector, on the floor
   stood on the ground under it. */
static int step(Actor *a)
{
    ActorMoveByVector(a, 0x02);
    if (own(a)->surface == 1)
        (void)ActorSnapToGround(a, NULL);
    a->resume = WALK;
    return 1;
}

/* Its walk set for its way: facing, flip and animation; then a step. */
static int walk_set(Actor *a)
{
    a->facing = k2b_direction(way(a));
    a->flip = k2b_flip(way(a));
    AnimStart(a, k2b_walk_anim(way(a)));
    return step(a);
}

/* Its turn: the leg the rest of it, its turning animation for 20 frames,
   then the other direction. */
static int turn(Actor *a)
{
    Kind2BOwn *o = own(a);

    set_word(o->leg, (uint16_t)(word(o->leg2) - word(o->leg)));
    a->flip = k2b_flip(way(a));
    AnimStart(a, k2b_turn_anim(way(a)));
    a->resume = TURNED;
    return 0x14;
}

/* The point at the offsets for its way in `offsets` (0 ahead, 1 ahead on
   its surface), as the original adds them: the y's ADC takes the x's carry
   ahead, not on its surface. */
static uint8_t tile_ahead(Actor *a, int on_surface)
{
    MapPoint p = { 0 };
    uint16_t dx = k2b_point_x(way(a), on_surface), dy = k2b_point_y(way(a), on_surface);
    uint16_t x = pixels(a->x);
    unsigned carry = on_surface ? 0 : ((uint32_t)dx + x) > 0xFFFF;

    p.x = (uint16_t)(dx + x);
    p.y = (uint16_t)(dy + pixels(a->y) + carry);
    return TileAt(a, &p);
}

/* Bit 3: its test of Aladdin, within its range across of +$48: a floor
   walker within his height turns to face him; one on a wall or a ceiling
   drops when he is level with it or below. */
static int test_player(Actor *a)
{
    Kind2BOwn *o = own(a);

    if ((uint16_t)(word(o->from_x) + word(o->range) - pixels(g.actors[0].x)) >= word(o->range2))
        return step(a);
    if (o->surface == 1) {
        if (!ActorNearPlayerY(a, 0x02) || ActorSideOfPlayer(a) == o->dir)
            return step(a);
        return turn(a);
    }
    if (!ActorPlayerBelow(a))
        return step(a);
    ActorMotionReset(a, 0x28);
    AnimStart(a, 3);
    a->resume = DROP;
    return 1;
}

/* A frame of its walk: still while Aladdin is within box 4 round it; else
   on to the end of its leg, a wall ahead ($70, a flat full tile) or the
   end of its surface, where it turns. */
static int walk(Actor *a)
{
    Kind2BOwn *o = own(a);

    if (ActorNearPlayer(a, 0x04))
        return 1;
    set_word(o->leg, (uint16_t)(word(o->leg) - 1));
    if (!word(o->leg))
        return turn(a);
    if (tile_ahead(a, 0) == 0x70)
        return turn(a);
    if (tile_ahead(a, 1) == TILE_KIND_EMPTY)
        return turn(a);
    if (!(a->spawn_byte & 0x08))
        return step(a);
    return test_player(a);
}

/* Landed from its drop: a floor walker facing him, its leg again, its
   range judged from here; 32 frames, then its walk. */
static int dropped(Actor *a)
{
    Kind2BOwn *o = own(a);

    o->surface = 1;
    set_word(o->leg, o->leg_kept[0]);
    set_word(o->from_x, pixels(a->x));
    o->dir = a->flip = ActorSideOfPlayer(a);
    AnimStart(a, 0);
    a->resume = DROPPED;
    return 0x20;
}

/* ---- the faller (+$07 bit 4) ---- */

static Actor *partner(Actor *a)
{
    return &g.actors[faller(a)->partner_low | faller(a)->partner_high << 8];
}

/* By its partner: facing it, slow, pose 1. */
static int by_partner(Actor *a)
{
    a->facing = pixels(a->x) < pixels(partner(a)->x) ? 1 : 0;
    a->flip = (uint8_t)(a->facing ^ 1);
    a->speed_x = (a->speed_x & ~0xFF) | 0x50;
    AnimStart(a, 1);
    a->resume = BY_PARTNER;
    return 1;
}

/*
 * @routine 83:CD47 Kind2BNearPartner
 * WHAT IT DOES: Says whether kind $2B's actor is past $1E of its
 * partner's x either way.
 */
static bool past_partner(Actor *a)
{
    return (uint16_t)(pixels(a->x) - pixels(partner(a)->x) + k2b_partner_half()) >= k2b_partner_span();
}

/* A frame by its partner: it ends when the partner's +$16 is set, jumps
   while its +$13 is 3, else walks to and fro within $1E of it. */
static int by_partner_frame(Actor *a)
{
    Kind2BFallerOwn *o = faller(a);
    Actor *p = partner(a);
    uint16_t x = pixels(a->x), y = pixels(a->y);

    if (p->own.bytes[OWN_16])
        return ActorEndPlaced(a);
    if (p->own.bytes[OWN_13] == 3) {
        a->speed_y = -0x400;
        a->accel_y = 0x40;
        a->resume = JUMP;
        return 1;
    }
    set_word(o->kept_x, x);
    set_word(o->kept_y, y);
    ActorMoveX(a);
    if (past_partner(a)) {
        /* back; facing left (nonzero) the original's ADC #4 runs with the
           compare's carry set: 5 */
        a->x = with_pixels(a->x, (uint16_t)(word(o->kept_x) + (a->facing ? 5 : 0)));
        a->y = with_pixels(a->y, word(o->kept_y));
        a->flip = a->facing;
        a->facing ^= 1;
    }
    return 1;
}

static int faller_start(Actor *a)
{
    Kind2BFallerOwn *o = faller(a);

    o->landed = 0;
    o->surface = 0;
    a->flags |= 0x80;
    AnimStart(a, 4);
    a->facing = 0;
    a->flip = 1;
    ActorSpeedXSetAccel(a, 1, 0x42);
    o->frames = 0x18;
    o->way_frames = 0x30;
    a->resume = WANDER;
    return 1;
}

/* A frame to and fro: Aladdin below it and within span 4 across starts
   its shake; else on, turning each +$41 frames for 16. */
static int wander(Actor *a)
{
    Kind2BFallerOwn *o = faller(a);

    ActorMoveX(a);
    if (ActorPlayerBelow(a) && ActorNearPlayerX(a, 0x04)) {
        o->shakes = 8;
        a->resume = SHAKE;
        return 6;
    }
    if (--o->frames)
        return 1;
    o->frames = o->way_frames;
    a->flip = a->facing;
    a->facing ^= 1;
    a->resume = WANDER_TURNED;
    return 0x10;
}

/* After each shake's 6 frames: turned; after 8, its fall from still. */
static int shake(Actor *a)
{
    a->flip ^= 1;
    if (--faller(a)->shakes)
        return 6;
    AnimStart(a, 3);
    a->accel_y = 0x20;
    a->speed_y = 0;
    a->resume = FALL;
    return 1;
}

/* A frame of its fall, never faster than 3 pixels a frame, until its
   partner says it has landed. */
static int fall(Actor *a)
{
    if (faller(a)->landed) {
        faller(a)->stays++;
        faller(a)->surface++;
        return by_partner(a);
    }
    if ((uint8_t)(a->speed_y >> 8) != 3)
        ActorAccelerateY(a);
    ActorStepY(a);
    return 1;
}

/*
 * @routine 83:A467 Kind2BStart
 * WHAT IT DOES: Actor kind $2B: a walker along a floor, a wall or a ceiling
 * (ways 0-7 of +$07: its surface and first direction), turning at each end
 * of its leg, at a wall and where its surface ends. With bit 3, when
 * Aladdin is within its range across: a ceiling or wall walker drops onto
 * him when he is level with or below it, then walks the floor facing him;
 * a floor walker turns to face him when he is within its height. With bit
 * 4: it walks to and fro on its own, and when he is below and within its
 * span across it shakes and falls until its partner (+$49) sets its +$4B;
 * then (never ended out of range) it walks to and fro within $1E of the
 * partner's x, jumps while the partner's +$13 is 3 and ends when the
 * partner's +$16 is set.
 */
static int start(Actor *a)
{
    Kind2BOwn *o = own(a);
    uint8_t n;

    o->stays = 0;
    if (a->spawn_byte & 0x10)
        return faller_start(a);
    a->flags |= 0x90;
    AnimStart(a, 2);
    o->dir = (int8_t)RandomNext() >= 0 ? 1 : 0;
    a->probes = k2b_probes();
    set_word(o->from_x, pixels(a->x));
    o->surface = (uint8_t)((a->spawn_byte & 0x07) >> 1);
    n = (uint8_t)(a->spawn_byte & 0x03);
    set_word(o->leg, k2b_leg(n));
    set_word(o->leg_kept, k2b_leg(n));
    set_word(o->leg2, (uint16_t)(k2b_leg(n) * 2));
    o->range[0] = k2b_range(n);
    o->range2[0] = (uint8_t)(k2b_range(n) << 1);
    o->range[1] = 0;
    o->range2[1] = 0;
    return turn(a);
}

/*
 * @routine 83:A725 Kind2BSecond
 * WHAT IT DOES: Kind $2B downed (its second state, as a shot downs it): its
 * hit pose, then it falls away, a floor walker in an arc (motion preset 3),
 * one on a wall or ceiling straight off, until it is out of range.
 */
static int downed(Actor *a)
{
    AnimStart(a, 3);
    if (!--own(a)->surface)
        ActorMotionPreset(a, 0x03);
    else
        ActorMotionReset(a, 0x38);
    a->resume = DOWNED;
    return 1;
}

int Kind2BCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return downed(a);
    case TURNED:
        own(a)->dir ^= 1;
        return walk_set(a);
    case WALK:
        return walk(a);
    case DROP:
        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        return dropped(a);
    case DROPPED:
        return walk_set(a);
    case WANDER:
        return wander(a);
    case WANDER_TURNED:
        /* the original goes back to its BRK: a frame before it moves */
        a->resume = WANDER;
        return 1;
    case SHAKE:
        return shake(a);
    case FALL:
        return fall(a);
    case BY_PARTNER:
        return by_partner_frame(a);
    case JUMP:
        ActorFall(a);
        if (ActorEndOutOfRange(a, 0x08) == ACTOR_ENDED)
            return ACTOR_ENDED;
        if (!faller(a)->landed)
            return 1;
        return by_partner(a);
    case DOWNED:
        if ((uint8_t)(a->speed_y >> 8) != 4)
            ActorAccelerateY(a);
        ActorMoveX(a);
        ActorStepY(a);
        if (ActorEndOutOfRange(a, 0x08) == ACTOR_ENDED)
            return ACTOR_ENDED;
        return 1;
    default:
        return kind_lost(a, "Kind2BCode");
    }
}
