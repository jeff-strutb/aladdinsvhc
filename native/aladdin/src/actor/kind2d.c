/*
 * kind2d.c: actor kind $2D, a thrower: it watches for Aladdin within its
 * reach ahead of it and throws copies of its kind at him (each with +$07
 * bit 7 set), which fly by their motion preset and hurt him in their
 * zone; what it does when hit, and when stunned.
 */
#include <stddef.h>

#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: the watch's frames left; a copy: frames left of its pose */
    uint8_t arc;                /* +$41: the throw's arc (the copy's +$07, bit 7 set); a copy: its pose */
    uint8_t arc_high;           /* +$42: the arc's high byte (always 0: he is 16 or more away) */
    uint8_t pose;               /* +$43: the throw's pose: 4 level, 3 Aladdin above */
    uint8_t unused44;
    uint8_t count;              /* +$45: the throw's frames left */
    uint8_t unused46;
    uint8_t first;              /* +$47: the throw's frames, by +$07 */
    uint8_t leaves;             /* +$48: the frame left of the throw its copy leaves at */
    uint8_t again;              /* +$49: set once a +$07 = 2 thrower has thrown twice */
    uint8_t unused4c[OWN_4F - OWN_4C];
    uint8_t copy;               /* +$4F: 0 a thrower, 1 a thrown copy (or a thrower hit once) */
    uint8_t stun_frames;        /* +$4A: stunned: frames left */
} Kind2DOwn;

_Static_assert(sizeof(Kind2DOwn) <= OWN_BYTES, "a view fits the own bytes");
_Static_assert(offsetof(Kind2DOwn, copy) == OWN_4F && offsetof(Kind2DOwn, stun_frames) == OWN_4A, "the view follows OwnPlace");

static Kind2DOwn *own(Actor *a) { return (Kind2DOwn *)a->own.bytes; }

static uint16_t player_x(void) { return pixels(g.actors[0].x); }
static uint16_t player_y(void) { return pixels(g.actors[0].y); }

enum {
    SHOWN = KIND2D_RESUME,      /* after its first frame shown */
    WATCH_START,                /* after a throw's rest: the watch from its start */
    WATCH,                      /* each frame of the watch */
    THROW,                      /* after the wait before a throw */
    THROW_FRAME,                /* each frame of a throw */
    FLY,                        /* a copy: each frame of its flight */
    STUNNED,                    /* each frame of its stun */
    KNOCKED,                    /* hit: knocked away (ActorKnockedAway, called again until it ends it) */
    KNOCKED_AIR,                /* hit facing left: knocked away in the air (ActorKnockedAwayResumed) */
    KNOCKED_COPY,               /* a hit copy (ActorKnockedAwayResumedB) */
};

/* The watch's first frame: 128 frames of it. */
static int watch(Actor *a)
{
    own(a)->frames = 0x80;
    a->resume = WATCH;
    return 1;
}

/*
 * @routine 83:A903 Kind2DRest
 * WHAT IT DOES: The thrower at rest (pose 7), then on watching.
 */
static int rest(Actor *a)
{
    AnimStart(a, 7);
    return watch(a);
}

/*
 * @routine 83:A988 Kind2DAfterThrow
 * WHAT IT DOES: After a throw: pose 7 for 72 frames, then on watching.
 */
static int after_throw(Actor *a)
{
    AnimStart(a, 7);
    a->resume = WATCH_START;
    return 0x48;
}

/* A throw ($83:A90C, in Kind2DRest's code): its pose; the copy leaves on
   the frame +$07 says, counted down from the throw's frames. */
static int throw_start(Actor *a)
{
    Kind2DOwn *o = own(a);

    o->first = k2d_throw_frames(a->spawn_byte);
    o->leaves = k2d_throw_leaves(a->spawn_byte);
    AnimStart(a, o->pose);
    a->spawn_at = (uint8_t)(((o->pose >> 1) & 0x02) + 0x16);
    o->count = o->first;
    a->resume = THROW_FRAME;
    return 1;
}

/* A frame of a throw: on its frame the copy leaves (sound $11; its +$07
   the arc with bit 7 set, its look the thrower's, its +$4F 1, one more of
   the kind counted); when the throw is done a +$07 = 2 thrower throws once
   more on the second arc, then it rests. */
static int throw_frame(Actor *a)
{
    Kind2DOwn *o = own(a);

    if (o->count == o->leaves) {
        uint8_t kept = a->spawn_byte;
        Actor *copy;

        SoundQueueAdd(0x11);
        a->spawn_byte = (uint8_t)(o->arc ^ 0x80);
        copy = ActorSpawn(a, a->kind);
        if (copy) {
            copy->look = a->look;
            own(copy)->copy = 1;
            g.kind_count[a->kind]++;
        }
        a->spawn_byte = kept;
    }
    if (--o->count)
        return 1;
    if (a->spawn_byte == 2 && !o->again) {
        o->arc = k2d_second_arc(o->arc);
        o->again++;
        return throw_start(a);
    }
    return after_throw(a);
}

/*
 * @routine 83:A875 Kind2DWatch
 * WHAT IT DOES: The thrower's watch: for up to 128 frames, each frame,
 * when Aladdin is near its height (from 64 above it to 96 below) and
 * within its reach ahead: pose 2, the arc by how far he is (3 more when he
 * is above it), the pose 3 or 4, a wait (1 frame, or 32 for +$07 = 0),
 * then the throw. At frame 96 it turns to face him in pose 1. The time
 * up, it rests (Kind2DRest).
 */
static int watch_frame(Actor *a)
{
    Kind2DOwn *o = own(a);
    uint16_t x = pixels(a->x), y = pixels(a->y);

    if ((uint16_t)(y + 0x40 - player_y()) < 0xA0) {
        uint8_t at = (uint8_t)(a->facing << 2);

        if ((uint16_t)(x + k2d_reach_from(at) - player_x()) < k2d_reach(at)) {
            uint16_t far = (uint16_t)(x - player_x());

            AnimStart(a, 2);
            if (far & 0x8000)
                far = (uint16_t)-far;
            far = (uint16_t)((uint16_t)(far - 0x10) >> 5);
            o->arc = (uint8_t)far;
            o->arc_high = (uint8_t)(far >> 8);
            o->pose = 4;
            if ((uint16_t)(y - 0x10) < player_y()) {
                o->arc = (uint8_t)(o->arc + 3);
                o->pose = 3;
            }
            a->resume = THROW;
            return a->spawn_byte ? 1 : 0x20;
        }
    }
    if (o->frames == 0x20) {
        a->facing = a->flip = ActorSideOfPlayer(a);
        AnimStart(a, 1);
    }
    if (!--o->frames)
        return rest(a);
    return 1;
}

/*
 * @routine 83:A995 Kind2DThrown
 * WHAT IT DOES: A thrown copy of kind $2D: it flies by the motion preset
 * its arc names, stepping through its poses every 13 frames up to pose
 * $0E, hurting Aladdin in its zone, until it is out of range. In levels
 * 0, 3 and $0B it is drawn with sprite priority $10.
 */
static int thrown(Actor *a)
{
    uint8_t level = (uint8_t)g.level;

    a->anim_flags |= 0x01;
    if (level == 0x00 || level == 0x03 || level == 0x0B)
        a->flags |= 0x10;
    a->spawn_byte &= 0x0F;
    ActorMotionPreset(a, (uint8_t)(0x17 + a->spawn_byte));
    own(a)->arc = k2d_copy_pose(a->spawn_byte);
    AnimStart(a, own(a)->arc);
    own(a)->frames = 0x0D;
    a->resume = FLY;
    return 1;
}

static int thrown_frame(Actor *a)
{
    Kind2DOwn *o = own(a);

    if (o->arc != 0x0E && !--o->frames) {
        o->frames = 0x0D;
        AnimStart(a, ++o->arc);
    }
    ActorFly(a);
    (void)ActorZoneHurtsPlayer(a, (uint8_t)((o->arc - 8) << 2));
    if (ActorEndOutOfRange(a, 0) == ACTOR_ENDED)
        return ACTOR_ENDED;
    return 1;
}

/*
 * @routine 83:A82F Kind2DStart
 * WHAT IT DOES: Actor kind $2D's start: a thrown copy (+$07 bit 7) goes to
 * Kind2DThrown; a thrower shows itself, faces Aladdin and rests
 * (Kind2DRest). Also kind $2C's second entry.
 * Not converted: the check the original makes first, that two bytes of the
 * actor loop's code (83:989E, 83:98AC) are not NOPs, which on a changed ROM
 * gives it a random kind: the record's @unreachable (the bytes are $D0 and
 * $80; Jeff, 2026-11-19: exempt), and the native game has no code bytes.
 */
int Kind2DStart(Actor *a)
{
    if (a->spawn_byte & 0x80)
        return thrown(a);
    own(a)->copy = 0;
    a->anim_flags |= 0x02;
    a->own_pictures = k2d_pictures();
    AnimStart(a, 0);
    ActorPictureQueue(a);
    a->resume = SHOWN;
    return 1;
}

/*
 * @routine 83:AA5A Kind2DHit
 * WHAT IT DOES: Kind $2D when hit (its state with bit 7 set): a thrown
 * copy is knocked away at once; a thrower, the first time, is knocked
 * away with sound $0F, or, while it faces left (bit 7 of its facing),
 * turned to face by bit 0 and knocked away in the air with sound 7.
 */
static int hit(Actor *a)
{
    if (own(a)->copy) {
        a->resume = KNOCKED_COPY;
        return ActorKnockedAwayResumedB(a, 5, 2, 3);
    }
    own(a)->copy = 1;
    if (a->facing & 0x80) {
        a->facing &= 0x01;
        a->flip = (uint8_t)(a->facing ^ 0x01);
        SoundQueueAdd(0x07);
        a->resume = KNOCKED_AIR;
        return ActorKnockedAwayResumed(a, 5, 2, 0x4C);
    }
    SoundQueueAdd(0x0F);
    a->resume = KNOCKED;
    return ActorKnockedAway(a, 5, 0x0F, 3);
}

/*
 * @routine 83:AA95 Kind2DStunned
 * WHAT IT DOES: Kind $2D in its third state: it stands stunned, facing
 * the way it is flipped, for 96 frames, then rests after a throw
 * (Kind2DAfterThrow). Aladdin bouncing on it, or a shot reaching it
 * while it faces left, sends it into its hit state (Kind2DHit); a shot
 * while it faces right starts it stunned again.
 */
static int stunned(Actor *a)
{
    a->facing = a->flip;
    a->flags &= 0x7F;
    AnimStart(a, 6);
    ActorPictureSend(a);
    own(a)->stun_frames = 0x60;
    a->resume = STUNNED;
    return 1;
}

static int stunned_frame(Actor *a)
{
    if (ActorBouncesPlayer(a)) {
        a->flags &= 0x7F;
        a->state = 0x8C;
        return hit(a);
    }
    if (ActorHitByShotCC(a) != NULL) {
        if (!(a->facing & 0x80))
            return stunned(a);
        return hit(a);
    }
    AnimStep(a);
    ActorPictureSend(a);
    if (--own(a)->stun_frames)
        return 1;
    a->flags |= 0x80;
    return after_throw(a);
}

/* Kind $2D's code: its three entries and where it goes on. */
int Kind2DCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return Kind2DStart(a);
    case ACTOR_START_SECOND:
        return hit(a);
    case ACTOR_START_THIRD:
        return stunned(a);
    case SHOWN:
        a->flags |= 0xD0;
        a->facing = a->flip = ActorSideOfPlayer(a);
        return rest(a);
    case WATCH_START:
        return watch(a);
    case WATCH:
        return watch_frame(a);
    case THROW:
        own(a)->again = 0;
        return throw_start(a);
    case THROW_FRAME:
        return throw_frame(a);
    case FLY:
        return thrown_frame(a);
    case STUNNED:
        return stunned_frame(a);
    case KNOCKED:
        return ActorKnockedAway(a, 5, 0x0F, 3);
    case KNOCKED_AIR:
        return ActorKnockedAwayResumed(a, 5, 2, 0x4C);
    case KNOCKED_COPY:
        return ActorKnockedAwayResumedB(a, 5, 2, 3);
    default:
        return kind_lost(a, "Kind2DCode");
    }
}

/*
 * @routine 83:AA47 ActorWaitHitsRange8
 * WHAT IT DOES: Kind $2D's wait routine: each frame it waits, its
 * animation and picture, the stomp-or-hurt and shot tests, then its end
 * when it is out of range by box 8.
 */
int ActorWaitHitsRange8(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    (void)ActorStompOrHurt(a);
    (void)ActorHitByShotCC(a);
    return ActorEndOutOfRange(a, 0x08);
}
