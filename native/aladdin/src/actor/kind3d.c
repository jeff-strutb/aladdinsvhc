/*
 * kind3d.c: actor kind $3D: with +$07 4 and up (and 5), a thing the player
 * can stand on and ride that bobs; with +$07 0-2 the keeper of one or two
 * of those (its children), which ends with them once it is far from the
 * range point. Its wait routine, Kind3DWait, is its contact with the
 * player (stands him on it, pushes him under it or off to its side), then
 * a shot that hits it turned back. Its second entry ends it.
 *
 * The helpers only this kind calls are its own: 83:C47E ActorPlayerBoxCount,
 * 83:C3CB ActorBobStep, 83:C3C5 ActorBobReturnStep, 83:C32A
 * ActorStartChild1E, 83:C332 ActorStartChild1E5, 83:C33A
 * ActorStartTwoChildren, 83:C4EC ChildStartListed.
 */
#include <stddef.h>
#include <string.h>

#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "player/player.h"

#define K3D_ON 0x3B             /* his y above it, standing on it: its y less this */
#define K3D_BELOW 0x3D          /* his y below it, pushed under: its y plus this */
#define K3D_LEFT 0x10           /* his x left of it, pushed off */
#define K3D_RIGHT 0x11          /* right of it */
#define K3D_BOX_HALF_X 16       /* its box: 16 pixels each side across */
#define K3D_BOX_HALF_Y 60       /* and 60 up and down */
#define K3D_SOUND 0x4E          /* the sound of its top */

/* its own bytes: as the riding thing keeps them (a keeper keeps its
   children's indices in g.actors at +$40 and +$42 and their number at +$44) */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a bob move */
    uint8_t x_was[2];           /* +$41-$42: x_count as it was the frame before */
    uint8_t y_was[2];           /* +$43-$44: y_count as it was */
    uint8_t x_count[2];         /* +$45-$46: nonzero: the player within its box across */
    uint8_t y_count[2];         /* +$47-$48: in the box up and down */
    uint8_t ridden;             /* +$49: nonzero: the player rides it */
    uint8_t unused4c[OWN_4A - OWN_4C];
    uint8_t unused4a;
    uint8_t no_hurt;            /* +$4B: nonzero: touching it from below does not hurt */
} Kind3DOwn;

_Static_assert(sizeof(Kind3DOwn) <= OWN_BYTES, "a view fits the own bytes");
_Static_assert(offsetof(Kind3DOwn, ridden) == OWN_49 && offsetof(Kind3DOwn, no_hurt) == OWN_4B, "the view follows OwnPlace");

static Kind3DOwn *own(Actor *a) { return (Kind3DOwn *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

enum {
    KEEPER = ACTOR_RESUME_OWN,  /* the keeper: each frame */
    KEEP_TOP,                   /* the riding thing: after its 40 frames still */
    UP_SET,                     /* after its first frame */
    UP,                         /* each frame of its 14 up */
    TOP_HELD,                   /* after its top's frame */
    DOWN_SET,                   /* after its 27 frames still */
    DOWN,                       /* each frame of its 56 down */
};

/*
 * @routine 83:C47E ActorPlayerBoxCount
 * WHAT IT DOES: Counts the player's nearness to the actor on each axis: the
 * word at +$45 steps up when the actor is within 16 pixels of the player in
 * x (-16 to 15), the word at +$47 when within 60 in y (-60 to 59).
 */
static void box_count(Actor *a)
{
    Kind3DOwn *o = own(a);
    Actor *player = &g.actors[0];

    if ((uint16_t)(pixels(a->x) - pixels(player->x) + K3D_BOX_HALF_X) < 2 * K3D_BOX_HALF_X)
        set_word(o->x_count, (uint16_t)(word(o->x_count) + 1));
    if ((uint16_t)(pixels(a->y) - pixels(player->y) + K3D_BOX_HALF_Y) < 2 * K3D_BOX_HALF_Y)
        set_word(o->y_count, (uint16_t)(word(o->y_count) + 1));
}

/* 83:C441: pushed off to the side, y_count up. */
static void side_push(Actor *a)
{
    Actor *player = &g.actors[0];
    uint16_t x = pixels(a->x);

    player->x = with_pixels(player->x, x >= pixels(player->x) ? (uint16_t)(x - K3D_LEFT) : (uint16_t)(x + K3D_RIGHT));
    set_word(own(a)->y_count, (uint16_t)(word(own(a)->y_count) + 1));
}

/*
 * @routine 83:C3E4 Kind3DPlayerContact
 * WHAT IT DOES: Actor kind $3D's contact with the player: once he comes
 * within its box from above he stands on it and rides it (it carries him
 * with its moves); from below he is pushed under it, and hurt when its +$07
 * is nonzero and its +$4B zero; touching it from the side he is pushed 16
 * pixels off to its left or 17 to its right.
 */
static void contact(Actor *a)
{
    Kind3DOwn *o = own(a);
    Actor *player = &g.actors[0];

    memcpy(o->x_was, o->x_count, 2);
    memcpy(o->y_was, o->y_count, 2);
    set_word(o->x_count, 0);
    set_word(o->y_count, 0);
    if (o->ridden) {
        PlayerMoveWithActor(a);
        (void)PlayerEnter8CFacing(a);
        box_count(a);
        if (!g.player_in_air && word(o->x_count) && word(o->y_count))
            g.player_on_actor++;
        else
            o->ridden = 0;
        return;
    }
    box_count(a);
    if (!word(o->x_count) || !word(o->y_count))
        return;
    if (o->y_was[0]) {
        side_push(a);
        return;
    }
    if (pixels(a->y) >= pixels(player->y)) {            /* he is above it: he stands on it */
        player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) - K3D_ON));
        g.player_on_actor = 1;
        o->ridden = 1;
        g.player_in_air = 0;
        return;
    }
    player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) + K3D_BELOW));   /* below it: pushed under */
    if (a->spawn_byte && !o->no_hurt)
        (void)PlayerEnter8CFacing(a);
    if (g.player_in_air) {
        set_word(o->y_count, 0);
        return;
    }
    side_push(a);
}

/*
 * @routine 83:C3CB ActorBobStep
 * @routine 83:C3C5 ActorBobReturnStep
 * WHAT IT DOES: One frame of the bob's move: the actor's y by the step of
 * entry `entry` of the bob's table (0 or 1 by its +$07 for the first move,
 * 2 or 3 for the return).
 */
static void bob_step(Actor *a, uint8_t entry)
{
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + k3d_bob_step(entry)));
}

/*
 * @routine 83:C4EC ChildStartListed
 * WHAT IT DOES: Starts a child of kind $3D at spawn point `at` with +$07
 * `byte07`, and keeps its record (its index in g.actors) in the actor's
 * list of children (+$40 + 2 * +$44), counting +$44 up.
 */
static void child_start(Actor *a, uint8_t at, uint8_t byte07)
{
    Actor *child;
    uint8_t n = a->own.bytes[OWN_44];

    a->spawn_at = at;
    a->spawn_byte = byte07;
    child = ActorCreateChild(a, 0x3D);
    if (!child)
        return;
    a->own.bytes[OWN_40 + 2 * n] = (uint8_t)(child - g.actors);
    a->own.bytes[OWN_44] = (uint8_t)(n + 1);
}

/*
 * @routine 83:C32A ActorStartChild1E
 * @routine 83:C332 ActorStartChild1E5
 * @routine 83:C33A ActorStartTwoChildren
 * WHAT IT DOES: Starts the keeper's children by its set (its +$07): set 0
 * one child at spawn point $1E (+$07 4); set 1 one at $1E (+$07 5); set 2
 * two, at $1C (+$07 5) and $1E (+$07 4), the free list's top two records
 * first put in address order (the lower on top).
 */
static void children_start(Actor *a, uint8_t set)
{
    switch (set) {
    case 0:
        child_start(a, 0x1E, 4);
        break;
    case 1:
        child_start(a, 0x1E, 5);
        break;
    default: {
        Actor **top = &g.free_pool[g.free_pool_top];

        if (top[0] >= top[-1]) {
            Actor *swap = top[0];

            top[0] = top[-1];
            top[-1] = swap;
        }
        child_start(a, 0x1C, 5);
        child_start(a, 0x1E, 4);
        break;
    }
    }
}

/*
 * @routine 83:C363 Kind3DBob
 * WHAT IT DOES: Actor kind $3D with +$07 4 and up: a thing the player can
 * ride (Kind3DWait) that bobs: 40 frames still, 14 frames up (its +$4B set
 * at the top, with sound $4E), 27 frames still, 56 frames back down,
 * again, keeping its place each frame for the ride.
 */
static int bob_start(Actor *a)
{
    Kind3DOwn *o = own(a);

    a->spawn_byte &= 0x01;
    a->anim_flags = 0x02;
    set_word(o->x_was, 0);
    set_word(o->y_was, 0);
    o->ridden = 0;
    AnimStart(a, a->spawn_byte);
    a->flags = 0x90;
    o->no_hurt = 0;
    a->resume = KEEP_TOP;
    return 1;
}

/*
 * @routine 83:C302 Kind3DStart
 * WHAT IT DOES: Actor kind $3D: with +$07 4 and up the bobbing thing the
 * player rides; else, when the free list holds records enough for the set,
 * it starts its children by set +$07 (0: one, 1: one, 2: two) and stays,
 * ended with them once it is far from the range point; with too few
 * records it ends.
 */
static int start(Actor *a)
{
    if (a->spawn_byte >= 4)
        return bob_start(a);
    a->own.bytes[OWN_44] = 0;
    if (g.free_pool_top < 0 || 2 * g.free_pool_top < k3d_records_needed(a->spawn_byte))
        return ActorEndPlaced(a);
    children_start(a, a->spawn_byte);
    a->resume = KEEPER;
    return 1;
}

int Kind3DCode(Actor *a)
{
    Kind3DOwn *o = own(a);

    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case KEEPER:
        return ActorEndFarWithChildren(a) == ACTOR_ENDED ? ACTOR_ENDED : 1;
    case KEEP_TOP:
        ActorKeepSpot(a);
        a->resume = UP_SET;
        return 0x28;
    case UP_SET:
        o->frames = 0x0E;
        a->resume = UP;
        return 1;
    case UP:
        ActorKeepSpot(a);
        bob_step(a, a->spawn_byte);
        if (--o->frames)
            return 1;
        o->no_hurt++;
        SoundQueueAdd(K3D_SOUND);
        a->resume = TOP_HELD;
        return 1;
    case TOP_HELD:
        ActorKeepSpot(a);
        a->resume = DOWN_SET;
        return 0x1B;
    case DOWN_SET:
        o->no_hurt = 0;
        o->frames = 0x38;
        a->resume = DOWN;
        return 1;
    case DOWN:
        ActorKeepSpot(a);
        bob_step(a, (uint8_t)(a->spawn_byte + 2));
        if (--o->frames)
            return 1;
        o->no_hurt = 0;
        a->resume = KEEP_TOP;           /* the loop's top */
        return 1;
    default:
        return kind_lost(a, "Kind3DCode");
    }
}

/*
 * @routine 83:C4A4 Kind3DWait
 * WHAT IT DOES: What actor kind $3D runs each frame it waits: its contact
 * with the player, then a shot that hits it turned back.
 */
int Kind3DWait(Actor *a)
{
    contact(a);
    (void)ActorTurnsShotBack(a);
    return ACTOR_GOES_ON;
}
