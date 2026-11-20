/*
 * kind3a.c: actor kind $3A, a hold Aladdin hangs from: once he comes into
 * its zone it takes him (his code PlayerHeld) and keeps him at the place
 * its picture sets, with his frame matching; when he is in the air or hurt
 * again it lets go and, 13 frames on, can take him again. Its wait routine
 * is Kind33Wait (kind_waits_rest.c: 83:BF7E Kind3AWait is the same bytes).
 * Its second entry in the original's table is kind $3B's start
 * (n5-kinds-e's): it hands its record to that code (ActorRunAs).
 */
#include "actor/kinds_c.h"
#include "actor/kinds_e.h"
#include "data/kinds_c_tables.h"
#include "player/player.h"
#include "system/system.h"

#define K3A_ZONES 0x0A          /* its zones: ActorZoneHasTestPoint's zone is (its picture + this) * 4 */
#define K3A_PAUSE 0x0D          /* frames of its pause after letting him go */
#define K3A_BOX_RIGHT 0x14      /* g.camera_window while he is held facing right */
#define K3A_BOX_LEFT 0x1E       /* and facing left */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t shown;              /* +$40: the frame it last gave Aladdin a picture for ($FF: none) */
    uint8_t unused41;
    uint8_t set;                /* +$42: set to 1 at its start; nothing reads it */
    uint8_t let_go;             /* +$43: frames left of its pause after letting him go */
} Kind3AOwn;

_Static_assert(sizeof(Kind3AOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind3AOwn *own(Actor *a) { return (Kind3AOwn *)a->own.bytes; }

enum {
    FREE = ACTOR_RESUME_OWN,    /* each frame until he comes into its zone */
    HELD,                       /* each frame he is held */
    LET_GO,                     /* each frame of its pause after letting him go */
};

/*
 * @routine 83:BF57 Kind3AHoldBox
 * WHAT IT DOES: Sets g.camera_window ($1A08) for Aladdin held: $14 facing
 * right, $1E facing left (his flip set).
 */
static void hold_box(void)
{
    g.camera_window = g.actors[0].flip ? K3A_BOX_LEFT : K3A_BOX_RIGHT;
}

/*
 * @routine 83:BF64 Kind3AHoldSpot
 * WHAT IT DOES: Keeps the place Aladdin is held at: its x and y plus the
 * hold's offsets by its picture, in g.player_saved_x and _y.
 */
static void hold_spot(Actor *a)
{
    g.player_saved_x = (uint16_t)(pixels(a->x) + k3a_hold_x(a->picture));
    g.player_saved_y = (uint16_t)(pixels(a->y) + k3a_hold_y(a->picture));
}

/* A frame free: animates; when he is not hurt, can land on an actor and
   holds no Down, and comes into its zone, it takes him. */
static int free_frame(Actor *a)
{
    AnimStep(a);
    if (g.player_hurt_count || (g.player_no_landing & 0x80) || (g.pad1_held & PAD_DOWN))
        return 1;
    if (!ActorZoneHasTestPoint(a, (uint8_t)((a->picture + K3A_ZONES) << 2)))
        return 1;
    g.actors[0].resume = PLAYER_AT_HELD;
    hold_spot(a);
    a->resume = HELD;
    return 1;
}

/* A frame he is held: it lets go when he is in the air or hurt; else keeps
   him at the place its picture sets, his frame by its own. */
static int held_frame(Actor *a)
{
    Kind3AOwn *o = own(a);
    Actor *player = &g.actors[0];

    AnimStep(a);
    hold_box();
    if (g.player_in_air || g.player_hurt_count) {
        o->let_go = K3A_PAUSE;
        a->resume = LET_GO;
        return 1;
    }
    player->x = with_pixels(player->x, (uint16_t)(pixels(a->x) + k3a_hold_x(a->picture)));
    player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) + k3a_hold_y(a->picture)));
    player->frame_time = 0x20;
    if (a->frame != o->shown) {
        o->shown = a->frame;
        player->frame = k3a_frame(a->frame, player->flip != 0);
        player->redraw = 0xFF;
        player->frame_time = 1;
    }
    return 1;
}

/*
 * @routine 83:BEB3 Kind3AStart
 * WHAT IT DOES: Actor kind $3A: a hold Aladdin hangs from. When he comes
 * into its zone (not hurt, he can land on an actor, no Down held) it takes
 * him: his code goes to PlayerHeld and it keeps him at the place its
 * picture sets, his frame by its own as he faces. When he is in the air or
 * hurt it lets go, waits 13 frames and can take him again.
 */
static int start(Actor *a)
{
    Kind33Look(a);
    own(a)->set = 1;
    own(a)->shown = 0xFF;
    a->resume = FREE;
    return 1;
}

int Kind3ACode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind3BStart, ACTOR_START);
    case FREE:
        return free_frame(a);
    case HELD:
        return held_frame(a);
    case LET_GO:
        AnimStep(a);
        if (--own(a)->let_go)
            return 1;
        g.camera_window = 0;
        a->resume = FREE;
        return 1;
    default:
        return kind_lost(a, "Kind3ACode");
    }
}
