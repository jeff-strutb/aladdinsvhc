/*
 * shot_hit.c: what an actor does when a shot (a record of the second pool:
 * the apples Aladdin throws) reaches it. Each answers the shot that
 * reached it, or NULL. A shot whose own byte $11 is negative does not hit
 * (it is harmless).
 */
#include "actor/actor_a.h"

static bool harmless(const Actor *shot)
{
    return (shot->own.bytes[OWN_11] & 0x80) != 0;
}

/* turned away from Aladdin by a harmless shot: his flip, turned when his
   frame ($2F) is $0B or more, bit 0 kept, with bit 7 set */
static void facing_away(Actor *a)
{
    const Actor *p = &g.actors[0];
    uint8_t side = p->frame >= 0x0B;

    a->facing = (uint8_t)(((side + p->flip) & 0x01) | 0x80);
}

/*
 * @routine 83:80FC ActorDownedByShot
 * WHAT IT DOES: When a shot reaches the actor, the actor is downed (state
 * $8C): a shot that hits stops too (state $8C) and the actor turns the way
 * it flew; a harmless one turns it from Aladdin; a small actor (kind $11)
 * marks the hit.
 * Also 83:80F8 ActorDownedByShotLong, for callers in other banks.
 */
Actor *ActorDownedByShot(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    Spawn11AtY(shot);
    if (harmless(shot)) {
        facing_away(a);
    } else {
        a->facing = shot->facing;
        ActorStateSet8C(shot);
    }
    ActorStateSet8CSelf(a);
    return shot;
}

/*
 * @routine 83:8135 ActorHitByShotCC
 * WHAT IT DOES: When a shot reaches the actor: a shot that hits stops
 * (state $8C), the actor turns the way it flew and goes into state $CC; a
 * harmless one downs the actor (state $8C), turned from Aladdin; a small
 * actor (kind $11) marks either.
 */
Actor *ActorHitByShotCC(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    Spawn11AtY(shot);
    if (harmless(shot)) {
        facing_away(a);
        ActorStateSet8CSelf(a);
    } else {
        a->facing = shot->facing;
        ActorStateSet8C(shot);
        ActorStateSetCC(a);
    }
    return shot;
}

/*
 * @routine 83:8171 ActorStopsShot
 * WHAT IT DOES: When a shot reaches the actor, a small actor (kind $11)
 * marks it; a shot that hits stops (state $8C); a harmless one downs the
 * actor (state $8C), facing Aladdin's flip with bit 7 set.
 * Also 83:816D ActorStopsShotLong, for callers in other banks.
 */
Actor *ActorStopsShot(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    Spawn11AtY(shot);
    if (harmless(shot)) {
        a->facing = (uint8_t)(g.actors[0].flip | 0x80);
        ActorStateSet8CSelf(a);
    } else {
        ActorStateSet8C(shot);
    }
    return shot;
}

/*
 * @routine 83:819A ActorTurnsShotBack
 * WHAT IT DOES: When a shot reaches the actor, turns it back: it flies the
 * other way, harmless (its $08 bit 7 set, it can no longer be hit), and a
 * small actor (kind $13) marks the hit.
 * Also 83:8196 ActorTurnsShotBackLong, for callers in other banks.
 */
Actor *ActorTurnsShotBack(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    Spawn13AtY(shot);
    shot->facing ^= 0x01;
    shot->resume = SHOT_AT_TURNED_BACK;
    shot->anim_flags |= 0x80;
    shot->flags &= 0x7F;
    return shot;
}

/*
 * @routine 83:81CF ActorEndsShot
 * WHAT IT DOES: When a shot reaches the actor, the shot ends: a small
 * actor (kind $13) marks the hit and the shot's code goes on where it
 * ends itself.
 * Also 83:81CB ActorEndsShotLong, for callers in other banks.
 */
Actor *ActorEndsShot(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    Spawn13AtY(shot);
    shot->resume = SHOT_AT_ENDS;
    return shot;
}

/*
 * @routine 83:81E8 ActorTakesShot
 * WHAT IT DOES: When a shot that hits reaches the actor, the actor faces
 * the way it flew (facing and flip) and the shot stops (state $8C), a
 * small actor (kind $11) marking it; a harmless shot is let pass. Answers
 * the shot that hit, or NULL.
 */
Actor *ActorTakesShot(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot || harmless(shot))
        return NULL;
    Spawn11AtY(shot);
    a->facing = shot->facing;
    a->flip = shot->facing;
    ActorStateSet8C(shot);
    return shot;
}

/*
 * @routine 83:8208 ActorDownedByHarmlessShot
 * WHAT IT DOES: When a shot reaches the actor: a harmless one downs it
 * (state $8C), turned from Aladdin, marked by a small actor (kind $11); a
 * shot that hits ends, marked by kind $13, and the actor is unharmed.
 */
Actor *ActorDownedByHarmlessShot(Actor *a)
{
    Actor *shot = ActorTouchesSecondPool(a);

    if (!shot)
        return NULL;
    if (harmless(shot)) {
        Spawn11AtY(shot);
        facing_away(a);
        ActorStateSet8CSelf(a);
    } else {
        Spawn13AtY(shot);
        shot->resume = SHOT_AT_ENDS;
    }
    return shot;
}
