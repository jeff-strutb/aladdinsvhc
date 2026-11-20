/*
 * player_touch.c: what an actor does when Aladdin touches it: hurts him,
 * or, when he comes down on it from above, he bounces off it (and may
 * stomp it).
 */
#include "actor/actor_a.h"
#include "audio/sound.h"
#include "player/player.h"

#define BOX_NO_LANDING 0x3C         /* g.player_box_wanted: the box no actor is landed on with */
#define STOMPED 0x8C                /* the state a stomped actor goes into */
#define STOMP_SOUND 0x0F

/* he is going up: his vertical speed's whole word negative */
static bool rising(void)
{
    return g.actors[0].speed_y < 0;
}

/* he can land on it: alive, not hurt, in the air, landing not barred */
static bool can_stand(void)
{
    return g.health && !g.player_hurt_count && g.player_in_air && !(g.player_no_landing & 0x80);
}

/*
 * @routine 83:840F ActorBouncesPlayer
 * WHAT IT DOES: When Aladdin comes down on the actor from above (and can
 * land), he bounces up off it; the actor is unharmed. Says whether he
 * did. Never with the box that lands on nothing.
 */
bool ActorBouncesPlayer(Actor *a)
{
    TestBoxTouch t;

    if (g.player_box_wanted == BOX_NO_LANDING || !actor_touches_test_box(a, &t))
        return false;
    if (!touch_from_above(&t, 10) || rising() || !can_stand())
        return false;
    PlayerStandOnActor(a, &t);
    return true;
}

/* After the test that tells above from not: hurt from the side or below;
   stomped from above when he can land: it can no longer be hit, goes into
   state $8C, one hit less, a mark at his feet. True when he was hurt. */
static bool stomp_or_hurt(Actor *a, const TestBoxTouch *t, uint16_t margin)
{
    if (!touch_from_above(t, margin)) {
        PlayerEnter8CFacing(a);
        return true;
    }
    if (!can_stand() || rising())
        return false;
    PlayerStandOnActor(a, t);
    a->flags &= 0x7F;
    a->state = STOMPED;
    a->own.bytes[OWN_11]--;
    ActorSpawnKind11AtPlayer();
    return false;
}

/*
 * @routine 83:83C9 ActorStompOrHurt
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt (true); coming down on it from above (and able to land) he
 * bounces off and stomps it: it goes into state $8C, loses one hit, and a
 * small actor marks the stomp.
 * Also 83:83C5 ActorStompOrHurtLong, for callers in other banks.
 */
bool ActorStompOrHurt(Actor *a)
{
    TestBoxTouch t;

    if (!actor_touches_test_box(a, &t))
        return false;
    return stomp_or_hurt(a, &t, 10);
}

/*
 * @routine 83:83AD ActorStompOrHurt16
 * WHAT IT DOES: As ActorStompOrHurt, with Aladdin counted above the actor
 * with a margin of 16 pixels rather than 10.
 */
bool ActorStompOrHurt16(Actor *a)
{
    TestBoxTouch t;

    if (!actor_touches_test_box(a, &t))
        return false;
    return stomp_or_hurt(a, &t, 16);
}

/*
 * @routine 83:8362 ActorStompedByPlayer
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt; coming down on it from above (and able to land: landing not
 * barred, in the air, not rising, not hurt) he stands on it and it is
 * stomped: where its code was is kept (OWN_35), it can no longer
 * be hit, state $8C, one hit less, sound $0F.
 * Also 83:835E ActorStompedByPlayerLong, for callers in other banks.
 */
void ActorStompedByPlayer(Actor *a)
{
    TestBoxTouch t;

    if (!actor_touches_test_box(a, &t))
        return;
    if (!touch_from_above(&t, 10)) {
        PlayerEnter8CFacing(a);
        return;
    }
    if ((g.player_no_landing & 0x80) || !g.player_in_air || rising() || g.player_hurt_count)
        return;
    PlayerStandOnActor(a, &t);
    a->own.bytes[OWN_35] = (uint8_t)a->resume;
    a->own.bytes[OWN_36] = (uint8_t)(a->resume >> 8);
    a->flags &= 0x7F;
    a->anim_flags |= 0x80;
    a->state = STOMPED;
    a->own.bytes[OWN_11]--;
    SoundQueueAdd(STOMP_SOUND);
}

/*
 * @routine 83:844A ActorBouncesOrHurtsPlayer
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt (2); coming down on it from above (and able to land) he is
 * stood on it (1); the actor is unharmed either way; else 0. Never with
 * the box that lands on nothing.
 * Also 83:8446 ActorBouncesOrHurtsPlayerLong, for callers in other banks.
 */
uint8_t ActorBouncesOrHurtsPlayer(Actor *a)
{
    TestBoxTouch t;

    if (g.player_box_wanted == BOX_NO_LANDING || !actor_touches_test_box(a, &t))
        return 0;
    if (!touch_from_above(&t, 10)) {
        PlayerEnter8CFacing(a);
        return 2;
    }
    if (!can_stand() || rising())
        return 0;
    PlayerStandOnActor(a, &t);
    return 1;
}
