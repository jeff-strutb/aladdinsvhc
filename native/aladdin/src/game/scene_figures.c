/*
 * scene_figures.c: scripted figures: actor kind $93, which walks in once
 * the scene's cue is given, and actor kind $72, which rides along at the
 * player's place in his moves until he takes move 5, then flies off. (The
 * record's file also holds the player's scene routines PlayerSceneStand,
 * PlayerEndingRide and PlayerSceneLand, the player's own: player/.)
 */
#include "actor/actor_own.h"
#include "data/game_tables.h"
#include "player/player.h"

#define K93_SPEED 0x72              /* ActorSpeedXSet's entry */
#define K93_STOP_X 0xA2             /* it walks left while its x pixels' low byte is this or more */
#define K93_LOOK_HIGH 0x02          /* +$2C */
#define K72_LEAVE_MOVE 5            /* the player's move at which it flies off */
#define K72_CHILD 0x7A              /* the child it starts as it flies off */
#define K72_FLY_MOTION 0x54         /* the motion preset it flies off on */
#define K72_PROBE_LOW 0xB5          /* its +$16: the word $D6B5 (no routine reads it as a probe list) */
#define K72_PROBE_HIGH 0xD6

enum {
    WAIT_CUE = ACTOR_RESUME_OWN,        /* kind $93: each frame until the scene's cue */
    WALK,                               /* each frame of its walk */
    STAY,                               /* for good */
    FOLLOW,                             /* kind $72: each frame at the player's place */
    FLY                                 /* each frame it flies off */
};

/* kind $72's own bytes */
#define ALONG_MOVE OWN_48               /* +$48: the player's move it last took up */

/*
 * @routine A5:F3F0 Kind93WalkIn
 * WHAT IT DOES: Actor kind $93: stands facing left until the scene's cue
 * (g.scene_cue_waited), then walks left until its x is below 162, takes its
 * second pose and stays.
 */
int Kind93Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        a->look = (uint16_t)((a->look & 0x00FF) | K93_LOOK_HIGH << 8);
        a->redraw = 0xFF;
        a->own_pictures = kind93_pictures();
        a->facing = 1;
        a->flip = 1;
        AnimStart(a, 0);
        a->resume = WAIT_CUE;
        return 1;
    case WAIT_CUE:
        if (!g.scene_cue_waited)
            return 1;
        ActorSpeedXSet(a, K93_SPEED);
        a->resume = WALK;
        return 1;
    case WALK:
        ActorMoveX(a);
        if ((uint8_t)pixels(a->x) >= K93_STOP_X)
            return 1;
        AnimStart(a, 1);
        a->resume = STAY;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:F40D Kind72RideAlong
 * WHAT IT DOES: Actor kind $72: rides along at the player's place, taking
 * his move as its animation, until he takes move 5; then it faces right,
 * starts a child of kind $7A, and flies off on motion $54 until it is off
 * the screen, and ends.
 */
int Kind72Code(Actor *a)
{
    Actor *p = &g.actors[0];
    uint8_t move;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x01;
        a->flags |= 0x90;
        a->own.bytes[OWN_16] = K72_PROBE_LOW;
        a->own.bytes[OWN_17] = K72_PROBE_HIGH;
        a->own_pictures = kind72_pictures();
        a->redraw = 0xFF;
        AnimStart(a, 0);
        a->own.bytes[ALONG_MOVE] = 0;
        a->resume = FOLLOW;
        return 1;
    case FOLLOW:
        a->x = with_pixels(a->x, pixels(p->x));
        a->y = with_pixels(a->y, pixels(p->y));
        move = g.player_move;
        if (move == a->own.bytes[ALONG_MOVE])
            return 1;
        if (move != K72_LEAVE_MOVE) {
            a->own.bytes[ALONG_MOVE] = move;
            AnimStart(a, move);
            a->anim_flags = kind72_flags(p->own.bytes[OWN_41]);
            return 1;
        }
        a->facing = 0;
        a->flip = 0;
        a->spawn_at = 0;
        ActorCreateChild8(a, K72_CHILD);
        AnimStart(a, 5);
        ActorMotionPreset(a, K72_FLY_MOTION);
        a->resume = FLY;
        return 1;
    default:                            /* FLY */
        ActorFly(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEndPlaced(a);
    }
}

/* ActorWaitAnimRange18's work: see kinds_game.c's row for kind $72 */

/*
 * @routine 83:F487 ActorWaitAnimRange18
 * WHAT IT DOES: Each frame it waits: its animation on, its picture sent,
 * and it ends once out of range (box $18).
 */
int ActorWaitAnimRange18(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    return ActorEndOutOfRange(a, 0x18);
}
