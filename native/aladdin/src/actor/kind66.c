/*
 * kind66.c: actor kind $66 by its $07: platforms that swing up and down,
 * rise when signalled, or a lift that carries Aladdin down holding him; a
 * gate that waits for its parent to end; or kind $6C's holder. Its wait
 * routine is ActorKind66Wait. Its second entry (84:8629 Kind73Start) is
 * kind $73's start: the record goes on in kind $73's code (Kind66Second).
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "actor/kinds_e.h"
#include "audio/sound.h"
#include "player/player.h"

/* its own bytes */
#define COUNT OWN_40            /* frames left of a stroke */
#define PERIOD OWN_41           /* a stroke's frames */
#define TOP OWN_42              /* its y pixels as it starts (word) */
#define GO OWN_44               /* nonzero: signalled to move (by the lift's Kind66TellOthers) */
#define SIGNAL OWN_45           /* what the lift sets in its kind's GO */
#define LOW OWN_46              /* its highest point: TOP less $38 (word) */
#define STEP OWN_48             /* Kind66Wobble's count */
#define PARENT OWN_49           /* the record that spawned it, an index in g.actors (+$49-$4A) */
#define HELD OWN_4B             /* nonzero: no touch or shot tests in its wait */
#define PAUSE OWN_4C            /* frames before the lift takes him again */

#define LIFT_ZONE 0x78          /* the lift's zone for him (ActorZoneHasTestPoint) */

static uint16_t own16(const Actor *a, unsigned at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own16_set(Actor *a, unsigned at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

static uint16_t y_of(const Actor *a) { return pixels(a->y); }
static void y_set(Actor *a, uint16_t y) { a->y = with_pixels(a->y, y); }

enum {
    SWING_UP = ACTOR_RESUME_OWN,    /* the swing: each frame up */
    SWING_DOWN_WAIT,                /* after 12 frames at the top */
    SWING_DOWN,                     /* each frame down */
    SWING_UP_WAIT,                  /* after 12 frames at the bottom */
    RISE_IDLE,                      /* the rise: waiting for its signal */
    RISE_UP,                        /* signalled: rising */
    RISE_HOLD,                      /* at its highest, while signalled */
    RISE_DOWN_WAIT,                 /* after $38 frames: a frame more */
    RISE_DOWN,                      /* coming back down */
    RISE_UP_WAIT,                   /* signalled again on the way down: after 24 frames, a frame more */
    LIFT_WATCH,                     /* the lift: waiting for him on it */
    LIFT_TAKE,                      /* taken: the next frame he is put on it */
    LIFT_HOLD,                      /* after 4 frames */
    LIFT_DOWN,                      /* each frame down with him */
    LIFT_BOTTOM,                    /* at the bottom, until he is let go */
    LIFT_UP,                        /* each frame back up */
    GATE                            /* the gate: until its parent ends */
};

/*
 * @routine 84:851C Kind66PlayerOnTop
 * WHAT IT DOES: Puts Aladdin on the lift: his x its x, his y its y + $32.
 */
static void player_on_top(const Actor *a)
{
    Actor *p = &g.actors[0];

    p->x = with_pixels(p->x, pixels(a->x));
    p->y = with_pixels(p->y, (uint16_t)(y_of(a) + 0x32));
}

/*
 * @routine 84:852E Kind66PlayerHeldAt
 * WHAT IT DOES: Keeps the place Aladdin hangs from the lift at: its x, its
 * y + $32 (g.player_saved_x, _y).
 */
static void player_held_at(const Actor *a)
{
    g.player_saved_x = pixels(a->x);
    g.player_saved_y = (uint16_t)(y_of(a) + 0x32);
}

/*
 * @routine 84:8540 Kind66TellOthers
 * WHAT IT DOES: Tells every live actor of its kind in the pool (records
 * from $0B5B, g.actors[8] on) its SIGNAL, as their GO, so the rising
 * platforms move with the lift.
 */
static void tell_others(const Actor *a)
{
    int i;

    for (i = ACTOR_RECORDS - ACTOR_POOL_SIZE; i < ACTOR_RECORDS; i++)
        if (g.actors[i].state && g.actors[i].kind == a->kind)
            g.actors[i].own.bytes[GO] = a->own.bytes[SIGNAL];
}

/*
 * @routine 84:8567 Kind66MarkTop
 * WHAT IT DOES: Keeps its height as it starts (TOP) and its highest point,
 * $38 above (LOW).
 */
static void mark_top(Actor *a)
{
    own16_set(a, TOP, y_of(a));
    own16_set(a, LOW, (uint16_t)(y_of(a) - 0x38));
}

/*
 * @routine 84:8575 Kind66Wobble
 * WHAT IT DOES: A platform's wobble as it moves: STEP counts round 0, 1,
 * 2, then $FF: at 0 its y a pixel down, at 2 a pixel up.
 */
static void wobble(Actor *a)
{
    uint8_t step = ++a->own.bytes[STEP];

    if (step == 0)
        y_set(a, (uint16_t)(y_of(a) + 1));
    else if (step == 2)
        y_set(a, (uint16_t)(y_of(a) - 1));
    else if (step != 1)
        a->own.bytes[STEP] = 0xFF;
}

/*
 * @routine 84:85B3 Kind66Spawn
 * WHAT IT DOES: Spawns a child of its kind with $07 `byte07` at its spawn
 * place (ActorSpawn), with its look and itself as the child's parent, and
 * counts the kind's live actors up.
 */
static void spawn(Actor *a, uint8_t byte07)
{
    uint8_t kept = a->spawn_byte;
    Actor *child;

    a->spawn_byte = byte07;
    child = ActorSpawn(a, a->kind);
    if (child) {
        child->look = a->look;
        own16_set(child, PARENT, (uint16_t)(a - g.actors));
        g.kind_count[a->kind]++;
    }
    a->spawn_byte = kept;
}

/* The lift's way back up, its signal off. */
static int lift_up(Actor *a)
{
    a->own.bytes[SIGNAL] = 0;
    tell_others(a);
    a->resume = LIFT_UP;
    return 1;
}

/*
 * @routine 84:839E Kind66Start
 * WHAT IT DOES: Actor kind $66, by its $07: bit 4, kind $6C's holder (pose
 * 3, its touches off, on as Kind6CStart); bit 3, a gate (Kind66Gate);
 * bit 2, a lift (child $0B at spawn point $5A, its wait routine off) that,
 * when Aladdin is in its zone $78 (not hurt, landing allowed), holds him
 * (PlayerSwitchHang) and carries him down 2 pixels a frame, sound $15 and
 * the rising platforms signalled past $10 below its top, until he lets go;
 * then back up a pixel a frame, and 16 frames before it takes him again;
 * bit 1, a platform (child $0A at $58, speed entry $48) that rises out of
 * reach while signalled, up to $38 above its start, stays while signalled,
 * and after $38 frames comes back down (signalled again: 24 frames, then
 * up); else a platform swinging up and down for good (child $0A at $56;
 * speed entry $48 for $80 frames each way, or with bit 0 $4B for $40;
 * 12 frames at each end). The moving platforms wobble (Kind66Wobble).
 * Also 84:8594 Kind66Gate: a piece that stays (pose 2, its wait routine
 * off) until the actor that spawned it has ended, then ends.
 */
int Kind66Start(Actor *a)
{
    uint8_t period;

    if (a->resume >= KIND6C_RESUME)
        return Kind6CStart(a);
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x03;
        a->flags |= 0x90;
        a->own.bytes[HELD] = 0;
        if (a->spawn_byte & 0x10) {
            AnimStart(a, 3);
            a->own.bytes[HELD]++;
            return Kind6CStart(a);      /* its JML: at kind $6C's start */
        }
        if (a->spawn_byte & 0x08) {
            a->anim_flags = 0x02;
            a->flags &= 0x7F;
            AnimStart(a, 2);
            a->resume = GATE;
            return 1;
        }
        a->own.bytes[GO] = 0;
        if (a->spawn_byte & 0x04) {
            a->spawn_at = 0x5A;
            spawn(a, 0x0B);
            a->flags &= 0x7F;
            AnimStart(a, 1);
            mark_top(a);
            a->own.bytes[SIGNAL] = 0;
            a->own.bytes[PAUSE] = 0;
            a->resume = LIFT_WATCH;
            return 1;
        }
        AnimStart(a, 0);
        a->own.bytes[STEP] = 0;
        if (a->spawn_byte & 0x02) {
            a->spawn_at = 0x58;
            spawn(a, 0x0A);
            ActorSpeedYSet(a, 0x48);
            mark_top(a);
            a->resume = RISE_IDLE;
            return 1;
        }
        a->spawn_at = 0x56;
        spawn(a, 0x0A);
        period = 0x80;
        if (a->spawn_byte & 0x01) {
            period = 0x40;
            ActorSpeedYSet(a, 0x4B);
        } else {
            ActorSpeedYSet(a, 0x48);
        }
        a->spawn_byte >>= 1;            /* the original's LSR of its $07 */
        a->own.bytes[COUNT] = period;
        a->own.bytes[PERIOD] = period;
        a->resume = SWING_UP;
        return 1;

    case SWING_UP:
        wobble(a);
        ActorStepYBack(a);
        if (--a->own.bytes[COUNT])
            return 1;
        a->resume = SWING_DOWN_WAIT;
        return 0x0C;
    case SWING_DOWN_WAIT:
        a->own.bytes[COUNT] = a->own.bytes[PERIOD];
        a->resume = SWING_DOWN;
        return 1;
    case SWING_DOWN:
        wobble(a);
        ActorStepY(a);
        if (--a->own.bytes[COUNT])
            return 1;
        a->resume = SWING_UP_WAIT;
        return 0x0C;
    case SWING_UP_WAIT:
        a->own.bytes[COUNT] = a->own.bytes[PERIOD];
        a->resume = SWING_UP;
        return 1;

    case RISE_IDLE:
        if (a->own.bytes[GO])
            a->resume = RISE_UP;
        return 1;
    case RISE_UP:
        if (!a->own.bytes[GO]) {
            a->resume = RISE_DOWN_WAIT;
            return 0x38;
        }
        if (y_of(a) < own16(a, LOW)) {
            a->resume = RISE_HOLD;
            return 1;
        }
        wobble(a);
        ActorStepYBack(a);
        return 1;
    case RISE_HOLD:
        if (a->own.bytes[GO])
            return 1;
        a->resume = RISE_DOWN_WAIT;
        return 0x38;
    case RISE_DOWN_WAIT:
        a->resume = RISE_DOWN;
        return 1;
    case RISE_UP_WAIT:
        a->resume = RISE_UP;
        return 1;
    case RISE_DOWN:
        if (a->own.bytes[GO]) {
            a->resume = RISE_UP_WAIT;
            return 0x18;
        }
        wobble(a);
        ActorStepY(a);
        if (y_of(a) >= own16(a, TOP))
            a->resume = RISE_IDLE;
        return 1;

    case LIFT_WATCH:
        if (ActorEndOutOfRange(a, 0x00) == ACTOR_ENDED)
            return ACTOR_ENDED;
        if (a->own.bytes[PAUSE]) {
            a->own.bytes[PAUSE]--;
            return 1;
        }
        if (g.player_hurt_count || (g.player_no_landing & 0x80) || !ActorZoneHasTestPoint(a, LIFT_ZONE))
            return 1;
        g.actors[0].resume = PLAYER_AT_SWITCH_HANG;
        player_held_at(a);
        a->resume = LIFT_TAKE;
        return 1;
    case LIFT_TAKE:
        player_on_top(a);
        a->resume = LIFT_HOLD;
        return 4;
    case LIFT_HOLD:
        a->resume = LIFT_DOWN;
        return 1;
    case LIFT_DOWN: {
        bool above;

        if (!(uint8_t)g.player_no_actor_landing)
            return lift_up(a);
        y_set(a, (uint16_t)(y_of(a) + 2));
        above = (uint16_t)(own16(a, TOP) + 0x10) >= y_of(a);
        player_on_top(a);
        if (above)
            return 1;
        SoundQueueAdd(0x15);
        a->own.bytes[SIGNAL]++;
        tell_others(a);
        a->resume = LIFT_BOTTOM;
        return 1;
    }
    case LIFT_BOTTOM:
        if ((uint8_t)g.player_no_actor_landing)
            return 1;
        return lift_up(a);
    case LIFT_UP:
        if (y_of(a) != own16(a, TOP)) {
            y_set(a, (uint16_t)(y_of(a) - 1));
            return 1;
        }
        a->own.bytes[PAUSE] = 0x10;
        a->resume = LIFT_WATCH;
        return 1;

    case GATE:
        if (g.actors[own16(a, PARENT)].state)
            return 1;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:944E ActorKind66Wait
 * WHAT IT DOES: Kind $66's wait routine, through 84:85DC Kind66WaitWork:
 * unless its HELD is set, a shot may end on it (ActorEndsShot), and
 * Aladdin touching it is hurt when the top of his box is not above its
 * middle ($20 below its y), else pushed aside (with his box a pixel
 * narrower when he is to its left); g.kind66_touched marks the touch. Not
 * touching (or HELD set), the mark is cleared and it is freed once out of
 * range.
 */
int ActorKind66Wait(Actor *a)
{
    if (!a->own.bytes[HELD]) {
        (void)ActorEndsShot(a);
        if (ActorHitsTestBoxIfActive(a)) {
            if ((uint16_t)(g.test_box_y - g.test_box_half_h) >= (uint16_t)(y_of(a) + 0x20)) {
                (void)PlayerEnter8CFacing(a);
                return ACTOR_GOES_ON;
            }
            g.kind66_touched = 1;
            if (!ActorSideOfPlayer(a)) {
                PlayerPushAside(a);
                return ACTOR_GOES_ON;
            }
            g.test_box_half_w--;
            PlayerPushAside(a);
            g.test_box_half_w++;
            return ACTOR_GOES_ON;
        }
    }
    g.kind66_touched = 0;
    return ActorFreeOutOfRange(a, 0x00) ? ACTOR_ENDED : ACTOR_GOES_ON;
}

/* Kind $66's second entry (state $80-$BF): the record goes on in kind
   $73's code from its start. */
int Kind66Second(Actor *a)
{
    return ActorRunAs(a, Kind73Start, ACTOR_START);
}
