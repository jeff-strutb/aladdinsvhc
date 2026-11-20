/*
 * guard_escort.c: the escort's moves, kind $08's records started with $07
 * 7 to 10 (and the record kind $69 sends to a restart): it waits at a spot
 * until Aladdin comes near, then follows him on foot, falling off ledges
 * and landing again; sent back to its restart point once he is far to its
 * left.
 */
#include "actor/guard.h"
#include "actor/kinds_d.h"

#define FAR_LEFT 0x00C0         /* how far left of the actor the player must be */

/* Its restart point `at`, a resume point of its code, for ActorRestart... */
static void restart_at(Actor *a, uint8_t at) { a->own.bytes[G_RESTART_AT] = at; }

/*
 * @routine A5:EF83 ActorRestartIfPlayerFarLeft
 * WHAT IT DOES: True when the player is more than 192 pixels left of the
 * actor: its code then goes back to its restart point.
 */
static bool far_left(const Actor *a)
{
    return (uint16_t)(pixels(g.actors[0].x) + FAR_LEFT) < pixels(a->x);
}

/* A frame of the follow: back to its restart point (run at once) when he
   is far to its left. Returns true when it restarted; `*r` its turn's wait. */
static bool restarted(Actor *a, int *r)
{
    if (!far_left(a))
        return false;
    a->resume = a->own.bytes[G_RESTART_AT];
    *r = GuardEscort(a);
    return true;
}

/*
 * @routine A5:EF3F EscortFollow
 * WHAT IT DOES: The escort waits until Aladdin is near (ActorNearPlayerX
 * 8), then follows him on foot (speed entry $12, animation 2), falling off
 * ledges (preset $7E) and landing again; whenever he is more than 192
 * pixels to its left it goes back to its restart point.
 */
static int follow(Actor *a, uint8_t restart)
{
    restart_at(a, restart);
    a->resume = G_ESCORT_FOLLOW;
    return 1;
}

/* At spot `spot`, flipped, in its first pose; then the follow, restarting
   at `restart`. */
static int wait_spot(Actor *a, uint8_t spot, uint8_t restart)
{
    a->flip = 1;
    AnimStart(a, 0);
    ActorPlaceAtSpot(a, spot);
    return follow(a, restart);
}

/* On foot: speed entry $12, animation 2. */
static int walk(Actor *a)
{
    ActorSpeedXFromA5(a, 0x12);
    ActorAnimChange(a, 0x02);
    a->resume = G_ESCORT_WALK;
    return 1;
}

/*
 * @routine A5:EF0C EscortStaySpot18
 * WHAT IT DOES: The escort stands at spot $18 for good.
 * Also A5:EF15 EscortWaitSpot0 (flipped, at spot 0, then the follow,
 * restarting at EscortWaitSpot4), A5:EF2B EscortWaitSpot4 (spot 4;
 * restarting at EscortStaySpot18), A5:EF97 EscortWaitSpot0C (spot $0C;
 * restarting at EscortStaySpot18), A5:EFAE EscortWaitSpot10 (spot $10;
 * restarting at its wait at spot $14, A5:EFC5: until Aladdin is near it
 * there, and on in code no script reaches, A5:EFDE, not converted).
 */
int GuardEscort(Actor *a)
{
    int r;

    switch (a->resume) {
    case G_ESCORT_STAY18:
        ActorPlaceAtSpot(a, 0x18);
        a->resume = G_ESCORT_STAYING;
        return 1;
    case G_ESCORT_SPOT0:
        return wait_spot(a, 0x00, G_ESCORT_SPOT4);
    case G_ESCORT_SPOT4:
        return wait_spot(a, 0x04, G_ESCORT_STAY18);
    case G_ESCORT_SPOT0C:
        return wait_spot(a, 0x0C, G_ESCORT_STAY18);
    case G_ESCORT_SPOT10:
        return wait_spot(a, 0x10, G_ESCORT_SPOT10_AGAIN);
    case G_ESCORT_SPOT10_AGAIN:
        a->flip = 1;
        AnimStart(a, 0);
        ActorPlaceAtSpot(a, 0x14);
        a->resume = G_ESCORT_SPOT10_NEAR;
        return 1;
    case G_ESCORT_SPOT10_NEAR:
        if (!ActorNearPlayerX(a, 0x08))
            return 1;
        /* on in A5:EFDE: code no script reaches */
        a->resume = G_ESCORT_STAYING;
        return 1;
    case G_ESCORT_FOLLOW:
        if (!ActorNearPlayerX(a, 0x08))
            return 1;
        a->flip = 0;
        return walk(a);
    case G_ESCORT_WALK:
        if (restarted(a, &r))
            return r;
        ActorMoveX(a);
        if (ActorSnapToGround(a, NULL))
            return 1;
        ActorMotionPreset(a, 0x7E);
        a->resume = G_ESCORT_FALL;
        return 1;
    case G_ESCORT_FALL:
        if (restarted(a, &r))
            return r;
        ActorAnimRiseFall(a);
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        return walk(a);
    default:                            /* G_ESCORT_STAYING */
        return 1;
    }
}
