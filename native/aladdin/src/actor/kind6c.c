/*
 * kind6c.c: actor kind $6C, a spot that takes Aladdin in: when he comes
 * within reach it holds him (his code at PlayerHeld) at its place until he
 * is let go, again and again. Its wait routine is ActorWaitRange00
 * (wait_step.c, shared with kinds $61 and $64). Its second entry (83:E5BB
 * Kind65Start) is kind $65's start: left aside.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "player/player.h"

enum {
    WATCH = KIND6C_RESUME,      /* each frame: is he in reach? (its own block: kind $66's holder runs this code too) */
    PLACE_HIM,                  /* taken: the next frame he is put at its place */
    HOLD,                       /* until he is let go */
    AGAIN                       /* after 16 frames: over */
};

/*
 * @routine 83:8865 ActorInReachOfPlayer
 * WHAT IT DOES: Whether Aladdin is within reach of it: his x plus 18 (8
 * when his flip is set) less its x below 26, then his y plus 2 less its y
 * below 31 (unsigned: a difference below 0 counts as far).
 */
static bool in_reach(const Actor *a)
{
    const Actor *p = &g.actors[0];
    uint16_t dx = (uint16_t)((p->flip ? 8 : 18) + pixels(p->x) - pixels(a->x));

    if (dx >= 26)
        return false;
    return (uint16_t)(pixels(p->y) + 2 - pixels(a->y)) < 31;
}

/*
 * @routine 83:E55E Kind6CStart
 * WHAT IT DOES: Actor kind $6C: its wait routine on; each frame, unless
 * Aladdin may not land on actors (bit 7 of g.player_no_landing), it is
 * level $0A and it cannot be hit (flags bit 6), or he is hurt, it looks
 * whether he is in reach (ActorInReachOfPlayer); when he is, his code goes
 * to PlayerHeld, its place is kept for him (g.player_saved_x, _y), and the
 * next frame he is put there; once g.player_no_actor_landing's low byte is
 * 0 again it waits 16 frames and starts over.
 */
int Kind6CStart(Actor *a)
{
    Actor *p = &g.actors[0];

    switch (a->resume) {
    case ACTOR_START:
    case AGAIN:
        a->flags |= 0x80;
        a->resume = WATCH;
        return 1;
    case WATCH:
        if (g.player_no_landing & 0x80)
            return 1;
        if ((g.level & 0xFF) == 0x0A && !(a->flags & 0x40))
            return 1;
        if (g.player_hurt_count || !in_reach(a))
            return 1;
        p->resume = PLAYER_AT_HELD;
        g.player_saved_x = pixels(a->x);
        g.player_saved_y = pixels(a->y);
        a->resume = PLACE_HIM;
        return 1;
    case PLACE_HIM:
        p->x = with_pixels(p->x, pixels(a->x));
        p->y = with_pixels(p->y, pixels(a->y));
        a->resume = HOLD;
        return 1;
    case HOLD:
        if ((uint8_t)g.player_no_actor_landing)
            return 1;
        a->resume = AGAIN;
        return 0x10;
    default:
        return 1;
    }
}
