/*
 * kind7b.c: actor kind $7B, a thing that waits for Aladdin to land on it,
 * then sends him on; and its wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "player/player.h"

#define SOUND 0x41

enum {
    WATCH = ACTOR_RESUME_OWN,   /* each frame until he lands on it */
    SENT,                       /* after its 32 frames */
    SENT_SOUND,                 /* after its 16 more */
};

/*
 * @routine 83:C572 Kind7BStart
 * WHAT IT DOES: Actor kind $7B, a thing that waits for Aladdin to land on
 * it from above (in the air, his speed down not negative, his test box on
 * it): then pose 1, his code sent on to PLAYER_AT_FLUNG, 32 frames, sound
 * $41, 16 more and back to waiting; without health left the touch sets
 * its own state to $8C instead. No script runs past the first frame.
 * Its second entry (83:C5D1 Kind7BSecond) ends it.
 */
int Kind7BStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        AnimStart(a, 0x00);
        a->resume = WATCH;
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case WATCH:
        if (!g.player_in_air || g.actors[0].speed_y < 0 || !ActorHitsTestBox(a))
            return 1;
        if (!g.health) {
            a->state = 0x8C;
            AnimStart(a, 0x00);
            return 1;
        }
        if ((uint16_t)(pixels(g.actors[0].y) + 0x10) >= pixels(a->y))
            return 1;
        AnimStart(a, 0x01);
        g.actors[0].resume = PLAYER_AT_FLUNG;
        a->resume = SENT;
        return 0x20;
    case SENT:
        SoundQueueAdd(SOUND);
        a->resume = SENT_SOUND;
        return 0x10;
    case SENT_SOUND:
        AnimStart(a, 0x00);
        a->resume = WATCH;
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:C5D5 Kind7BWait
 * WHAT IT DOES: Kind $7B's wait: its animation on, then its end when it is
 * out of range by box $60.
 */
int Kind7BWait(Actor *a)
{
    AnimStep(a);
    return ActorEndOutOfRange(a, 0x60);
}
