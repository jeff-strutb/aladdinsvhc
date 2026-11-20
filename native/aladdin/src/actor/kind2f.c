/*
 * kind2f.c: actor kind $2F, which lies still until it can be hit, rises,
 * then walks the ground and hops where it ends; and its downed state. Its
 * wait routine (ActorWaitHitsRange0, here) counts the shots that down it.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "map/map.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a pose (the low byte of the walk's word) */
    uint8_t frames_high;        /* +$41: the walk's frames' high byte */
    uint8_t hits;               /* +$42: counted up by its wait routine when a shot downs it */
} Kind2FOwn;

_Static_assert(sizeof(Kind2FOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind2FOwn *own(Actor *a) { return (Kind2FOwn *)a->own.bytes; }

enum {
    LIE = ACTOR_RESUME_OWN,     /* each frame until it can be hit */
    BEFORE_RISE,                /* each frame of the wait before it rises */
    RISE_POSE,                  /* each frame of pose 1 before the rise */
    RISE,                       /* each frame of the rise and fall */
    LANDED,                     /* each frame of pose 6 after a landing */
    LANDED_2,                   /* each frame of pose 2 after it */
    WALK,                       /* each frame of the walk */
    STAND,                      /* each frame of pose 2 when the walk ran out */
    HOP_POSE,                   /* each frame of pose 2 before a hop */
    HOP,                        /* each frame of a hop */
    DOWNED,                     /* its second entry: each frame of its 10 */
    DOWNED_FLY,                 /* each frame of its flight off */
};

static int pose_for(Actor *a, uint8_t pose, uint8_t frames, int next)
{
    AnimStart(a, pose);
    own(a)->frames = frames;
    a->resume = (uint16_t)next;
    return 1;
}

/* Landed: pose 6 for 5 frames, then pose 2 for 16, then the walk. */
static int landed(Actor *a)
{
    return pose_for(a, 6, 0x05, LANDED);
}

/* The walk: pose 3 (4 with +$07 bit 2) for up to 256 frames, at speed 6
   (9). */
static int walk_start(Actor *a)
{
    int quick = a->spawn_byte & 0x04;

    AnimStart(a, quick ? 4 : 3);
    own(a)->frames = 0x00;
    own(a)->frames_high = 0x01;
    ActorSpeedXSet(a, quick ? 0x09 : 0x06);
    a->resume = WALK;
    return 1;
}

static int stand(Actor *a);

/* A frame of the walk: on along the ground; where the ground ahead ends
   it hops; at a wall it turns; the walk's time up, it stands. */
static int walk(Actor *a)
{
    Kind2FOwn *o = own(a);
    ActorMoveX(a);
    (void)ActorSnapToGround(a, NULL);
    if (ActorMapAhead5Down12(a) == 0)
        return pose_for(a, 2, 0x08, HOP_POSE);
    if (ActorOutOfWallProbe4(a)) {
        a->facing ^= 0x01;
        a->flip = a->facing;
    }
    if (o->frames-- == 0)
        o->frames_high--;
    if (o->frames || o->frames_high)
        return 1;
    AnimStart(a, 2);
    o->frames = 0x10;
    return stand(a);
}

/* Pose 2 when the walk ran out: its 16 frames counted down before each
   wait (so 15 waits), then pose 0 and $B4 frames' wait before it rises
   again. */
static int stand(Actor *a)
{
    a->resume = STAND;
    if (--own(a)->frames)
        return 1;
    AnimStart(a, 0);
    own(a)->frames = 0xB4;
    a->resume = BEFORE_RISE;
    return 1;
}

/*
 * @routine 83:B4BE Kind2FStart
 * WHAT IT DOES: Actor kind $2F: it lies still until it can be hit (bit 6
 * of its flags), rises with sound $16 and falls back to the ground, then
 * walks along it for up to 256 frames, turning at walls, and hops when the
 * ground ahead ends, again and again. When the walk runs out it stands
 * and waits $B4 frames before rising again.
 * Not converted: its check for a copier, whose save RAM reads back where
 * the retail board's open bus does not (the original then puts the player
 * at x 0): on the retail board the read gives $70, its INC $71, and the
 * read again $70, so the branch never runs (plan section 1 item 8).
 */
static int start(Actor *a)
{
    uint8_t way = (uint8_t)(a->spawn_byte & 0x03);

    a->anim_flags = 0x02;
    a->flags = 0x90;
    a->probes = k2f_probes();
    a->facing = a->flip = way ? (uint8_t)(way >> 1) : ActorSideOfPlayer(a);
    own(a)->hits = 0;
    AnimStart(a, 0);
    a->resume = LIE;
    return 1;
}

/*
 * @routine 83:B643 Kind2FSecond
 * WHAT IT DOES: Actor kind $2F in its second state (downed): it is knocked
 * up with sound $0F (no sound when its wait counted a shot and it faces
 * right),
 * plays its downed animation for 10 frames, then flies off until it is
 * out of range.
 */
static int downed(Actor *a)
{
    ActorMotionPreset(a, 0x1F);
    AnimStart(a, 7);
    own(a)->frames = 0x0A;
    if (!own(a)->hits || (a->facing & 0x80))
        SoundQueueAdd(0x0F);
    if (a->facing & 0x80)
        a->facing &= 0x7F;
    a->resume = DOWNED;
    return 1;
}

int Kind2FCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return downed(a);
    case LIE:
        if (!(a->flags & 0x40))
            return 1;
        own(a)->frames = 0x08;
        a->resume = BEFORE_RISE;
        return 1;
    case BEFORE_RISE:
        if (--own(a)->frames)
            return 1;
        return pose_for(a, 1, 0x10, RISE_POSE);
    case RISE_POSE:
        if (--own(a)->frames)
            return 1;
        ActorMotionPresetY(a, 0x1E);
        SoundQueueAdd(0x16);
        a->resume = RISE;
        return 1;
    case RISE:
        ActorFall(a);
        ActorFallSpeedCap(a);
        if (!ActorOutOfGround(a))
            return 1;
        return landed(a);
    case LANDED:
        if (--own(a)->frames)
            return 1;
        return pose_for(a, 2, 0x10, LANDED_2);
    case LANDED_2:
        if (--own(a)->frames)
            return 1;
        return walk_start(a);
    case WALK:
        return walk(a);
    case STAND:
        return stand(a);
    case HOP_POSE:
        if (--own(a)->frames)
            return 1;
        AnimStart(a, 5);
        ActorMotionPreset(a, 0x1D);
        a->resume = HOP;
        return 1;
    case HOP:
        ActorFly(a);
        ActorFallSpeedCap(a);
        (void)ActorOutOfWallProbe4(a);
        if (!ActorOutOfGround(a))
            return 1;
        return landed(a);
    case DOWNED:
        AnimStep(a);
        if (--own(a)->frames)
            return 1;
        a->resume = DOWNED_FLY;
        return 1;
    case DOWNED_FLY:
        AnimStep(a);
        ActorFly(a);
        if (ActorEndOutOfRange(a, 0) == ACTOR_ENDED)
            return ACTOR_ENDED;
        return 1;
    default:
        return kind_lost(a, "Kind2FCode");
    }
}

/*
 * @routine 83:B630 ActorWaitHitsRange0
 * WHAT IT DOES: Kind $2F's wait routine: each frame it waits, the
 * stomp-or-hurt and shot tests (+$42 counted up when a shot downs it), its
 * animation, then its end when it is out of range by box 0.
 */
int ActorWaitHitsRange0(Actor *a)
{
    (void)ActorStompOrHurt(a);
    if (ActorDownedByShot(a) != NULL)
        own(a)->hits++;
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x00);
}
