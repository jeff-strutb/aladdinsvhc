/*
 * kind53.c: actor kind $53, a thing another actor holds, then lets drop:
 * it falls, bounces along the ground and breaks; or, started in its second
 * state, thrown, it flies until it reaches the ground and breaks. Its wait
 * routine is Kind53Wait (kind_waits.c).
 *
 * Its holder and its counter (a record counting its kind's live ones, at
 * its +$4B) are named by their index in g.actors, in its +$4E-$4F and
 * +$4C-$4D, which its maker sets.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"

/* its own bytes */
#define BOUNCES OWN_40          /* bounces left; then frames left breaking */
#define LET_GO OWN_45           /* nonzero: its holder has let it go */
#define COUNTER OWN_4C          /* the record counting its kind's live ones: its index (+$4C-$4D) */
#define HOLDER OWN_4E           /* the record of the actor holding it: its index (+$4E-$4F) */
#define LIVE OWN_4B             /* in the counting record: how many are live */

enum {
    HELD = ACTOR_RESUME_OWN,    /* each frame held */
    FALL,                       /* let go: each frame of its fall */
    FLY,                        /* each frame of a bounce's flight */
    BREAK,                      /* each frame breaking */
    THROWN                      /* each frame of its throw (its second state) */
};

static uint16_t own16(const Actor *a, unsigned at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

/* It breaks: animation 8 (with sound $24 when it can be hit) for $28
   frames, then its kind's live count in its counter down one. */
static int start_break(Actor *a)
{
    AnimStart(a, 8);
    if ACTOR_ON_SCREEN(a)
        SoundQueueAdd(0x24);
    a->own.bytes[BOUNCES] = 0x28;
    a->resume = BREAK;
    return 1;
}

/* A bounce's flight, from a speed already set: one frame of it. True when
   it is in the ground again. */
static bool fly_frame(Actor *a)
{
    ActorFly(a);
    if (ActorOutOfWallProbe4(a)) {
        a->facing ^= 0x01;
        a->flip = a->facing;
    }
    return ActorOutOfGround(a);
}

/* Landed: bounce again (sound $30 when it can be hit, speed entry $4E up),
   or, the eighth time, stop being hittable and break. */
static int bounce(Actor *a)
{
    if (--a->own.bytes[BOUNCES]) {
        if ACTOR_ON_SCREEN(a)
            SoundQueueAdd(0x30);
        ActorSpeedYSet(a, 0x4E);
        a->resume = FLY;
        return 1;
    }
    a->flags &= 0x7F;
    return start_break(a);
}

/*
 * @routine 83:D566 Kind53Held
 * WHAT IT DOES: Actor kind $53 started new: held by another actor, at the
 * place its holder's animation frame gives (from its holder's place),
 * facing as the holder, until the holder lets it go (its holder's +$45,
 * or its wait routine sets its own +$45); then it falls to the ground
 * (gravity $10), sound $30, bounces up (preset $42), and 8 times more
 * (speed entry $4E, sound $30 when it can be hit), turned round at walls,
 * and breaks.
 * Also 83:D694 Kind53Thrown (its second entry): thrown (preset $4B), it
 * flies animating until it reaches the ground, then breaks. And the break
 * (83:D6AC): animation 8, sound $24 when it can be hit, $28 frames of it,
 * its counter's live count down one, then it ends.
 * (83:D665 Kind53EndOutOfRange is dead: nothing names it.)
 */
int Kind53Held(Actor *a)
{
    const Actor *holder = &g.actors[own16(a, HOLDER)];

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x03;
        a->flags = 0x90;
        a->probes = k53_probes();
        a->look = 0x0600;
        a->own.bytes[LET_GO] = 0;
        a->facing = a->flip = 1;
        AnimStart(a, 7);
        a->resume = HELD;
        return 1;
    case HELD:
        if (!a->own.bytes[LET_GO]) {
            a->facing = holder->facing;
            a->flip = holder->flip;
            a->x = with_pixels(a->x, (uint16_t)(k53_held_x(holder->frame) + pixels(holder->x)));
            a->y = with_pixels(a->y, (uint16_t)(k53_held_y(holder->frame) + pixels(holder->y)));
            a->own.bytes[LET_GO] = holder->own.bytes[LET_GO];
            if (!a->own.bytes[LET_GO])
                return 1;
        }
        ActorMotionReset(a, 0x10);
        a->facing = a->flip = (uint8_t)(a->flip & 0x01);
        a->resume = FALL;
        return 1;
    case FALL:
        ActorFall(a);
        if (!ActorOutOfGround(a))
            return 1;
        SoundQueueAdd(0x30);
        ActorMotionPreset(a, 0x42);
        a->own.bytes[BOUNCES] = 0x09;
        a->resume = FLY;
        return 1;
    case FLY:
        if (!fly_frame(a))
            return 1;
        return bounce(a);
    case ACTOR_START_SECOND:
        ActorMotionPreset(a, 0x4B);
        a->resume = THROWN;
        return 1;
    case THROWN:
        AnimStep(a);
        ActorMove(a);
        if (!ActorOutOfGround(a))
            return 1;
        return start_break(a);
    case BREAK:
        AnimStep(a);
        if (--a->own.bytes[BOUNCES])
            return 1;
        g.actors[own16(a, COUNTER)].own.bytes[LIVE]--;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}
