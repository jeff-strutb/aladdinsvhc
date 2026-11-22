/*
 * kind79.c: actor kind $79, which starts below the point actors are kept
 * in range of and rises until the player takes hold of it (its +$42 set),
 * then carries him up; landed on or shot, it falls away.
 */
#include <stdio.h>
#include "video/view.h"

#include "actor/actor_own.h"
#include "audio/sound.h"
#include "player/player.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: the second entry's sink, frames left */
    uint8_t hit;                /* +$41: counted up by its second entry; nonzero, its wait routine tests nothing */
    uint8_t taken;              /* +$42: nonzero, it is taken up and goes on */
    uint8_t shots;              /* +$43: counted up each frame a shot reaches it */
    uint8_t unused_44;          /* +$44: Kind79SlotClear's offset (not kept) */
    uint8_t first_y_low;        /* +$45: the first of a pair: its y pixels as it was placed, low byte */
    uint8_t first_y_high;       /* +$46: high byte */
    uint8_t wait;               /* +$47: frames left before it can be taken up (counted down) */
    uint8_t unused_48;          /* +$48 */
    uint8_t low;                /* +$49: nonzero, it came up past y $0032: state $8C each frame */
} K79Own;

_Static_assert(sizeof(K79Own) <= sizeof(((Actor *)0)->own.bytes), "a view fits the own bytes");

static inline K79Own *k79(Actor *a) { return (K79Own *)a->own.bytes; }

enum {
    RISE = ACTOR_RESUME_OWN,    /* each frame: up, until taken */
    TAKEN,                      /* the frame after he took hold */
    DIP,                        /* it dips with him and comes back up */
    CARRY,                      /* it carries him as it rises */
    SINK,                       /* second entry: landed on, it sinks 8 frames */
    FALL_SET,                   /* after its last pose's 6 frames */
    FALL_AWAY                   /* each frame, for good */
};

#define RISE_SPEED 0x6F         /* its vertical speed's entry as it rises */
#define DIP_SPEED 0x6C          /* as it dips with him, or sinks */
#define DIP_GRAVITY 0x04
#define HOLD_BELOW 0x2B         /* the player's y below its own */
#define LET_GO_WAIT 0x10        /* frames before it can be taken up again */
#define TAKE_ZONE 0x80          /* the zone it is taken up by */

/*
 * @routine 83:F5FD Kind79PlayerToIt
 * WHAT IT DOES: Puts Aladdin at actor kind $79's place: its x, and its y
 * plus $2B pixels.
 */
static void Kind79PlayerToIt(Actor *a)
{
    g.actors[0].x = with_pixels(g.actors[0].x, pixels(a->x));
    g.actors[0].y = with_pixels(g.actors[0].y, (uint16_t)(pixels(a->y) + HOLD_BELOW));
}

/*
 * @routine 83:F610 Kind79Held
 * WHAT IT DOES: Whether Aladdin still holds on to actor kind $79 (his box
 * wanted $07-$0F): if so it is what holds him (his record's +$37); if not
 * it lets him go: pose 0, no longer taken, nothing holds him, and 16
 * frames before it can be taken again.
 */
static int Kind79Held(Actor *a)
{
    if (g.player_box_wanted >= 0x07 && g.player_box_wanted < 0x10) {
        g.actors[0].next = a;
        return 1;
    }
    AnimStart(a, 0);
    k79(a)->taken = 0;
    g.actors[0].next = NULL;
    k79(a)->wait = LET_GO_WAIT;
    return 0;
}

/*
 * @routine 83:F683 Kind79SlotClear
 * WHAT IT DOES: In the original, clears the word at $1C77 + its +$44 as it
 * leaves. Not kept: nothing in the game reads $1C77-$1C7C (no operand of
 * asm/aladdin outside this routine names them) and nothing writes +$44
 * (its own bytes start at 0), so the store is lost.
 */

/* Its last pose (3) with sound $2D, then after 6 frames it falls away. */
static int pose_and_fall(Actor *a)
{
    AnimStart(a, 3);
    SoundQueueAdd(0x2D);
    a->resume = FALL_SET;
    return 6;
}

/*
 * @routine 83:F541 Kind79Rise
 * WHAT IT DOES: Actor kind $79. An even +$07 (the first of a pair): +$07
 * made odd, its placed y kept at +$45, 32 pixels below the point actors
 * are kept in range of (g.range_y), its wait routine on (flags $90), view
 * box 8; an odd +$07: 112 pixels below. Then anim_flags 2, pose 0, sent up
 * (speed entry $6F), +$41, +$42, +$43 and +$49 cleared, and each frame it
 * rises (ActorStepYBack) until taken up (+$42). Taken: the player's code
 * goes on at PlayerHeld, held at its x and its y + $2B (g.player_saved_x,
 * _y); the next frame pose 2, the player at it, and it dips (speed entry
 * $6C, gravity 4, against the pull) with him while he holds on, until it
 * is rising slower than $60 pixels a frame; then sent up again ($6F). Let
 * go meanwhile: the rise again. Else it carries him as it rises while he
 * holds on, and rises again when he lets go.
 * Its second entry is 83:F6F2 Kind79Second (below), in this function.
 */
int Kind79Rise(Actor *a)
{
    K79Own *k = k79(a);
    uint16_t px;

    switch (a->resume) {
    case ACTOR_START:
        if (!(a->spawn_byte & 0x01)) {
            a->spawn_byte++;
            px = pixels(a->y);
            k->first_y_low = (uint8_t)px;
            k->first_y_high = (uint8_t)(px >> 8);
            a->y = with_pixels(a->y, (uint16_t)(g.range_y + 0x20));
            a->flags = 0x90;
            a->view_box = 0x08;
        } else {
            a->y = with_pixels(a->y, (uint16_t)(g.range_y + 0x70));
        }
        a->anim_flags = 0x02;
        AnimStart(a, 0);
        ActorSpeedYSet(a, RISE_SPEED);
        k->hit = 0;
        k->taken = 0;
        k->shots = 0;
        k->low = 0;
        a->resume = RISE;
        return 1;
    case RISE:
        ActorStepYBack(a);
        if (!k->taken)
            return 1;
        g.actors[0].resume = PLAYER_AT_HELD;
        g.player_saved_x = pixels(a->x);
        g.player_saved_y = (uint16_t)(pixels(a->y) + HOLD_BELOW);
        a->resume = TAKEN;
        return 1;
    case TAKEN:
        AnimStart(a, 2);
        Kind79PlayerToIt(a);
        ActorSpeedYSetGravity(a, DIP_GRAVITY, DIP_SPEED);
        a->resume = DIP;
        return 1;
    case DIP:
        ActorFallAgainst(a);
        if (Kind79Held(a))
            Kind79PlayerToIt(a);
        if (a->speed_y >= 0)
            return 1;
        if ((uint16_t)(a->speed_y >> 8) < 0xFFA0)
            return 1;
        ActorSpeedYSet(a, RISE_SPEED);
        a->resume = k->taken ? CARRY : RISE;
        return 1;
    case CARRY:
        ActorStepYBack(a);
        if (!Kind79Held(a)) {
            a->resume = RISE;
            return 1;
        }
        Kind79PlayerToIt(a);
        return 1;

    /*
     * @routine 83:F6F2 Kind79Second
     * WHAT IT DOES: Actor kind $79 in a state with bit 7 set (landed on, or
     * hit): +$41 up. Landed on (+$49 and +$43 both 0): sound $0F, pose 1,
     * it sinks (speed entry $6C, gravity 4, against the pull) for 8 frames.
     * Else, when it was taken up and Aladdin has health left, his code goes
     * on at PlayerFall and it is no longer taken; its wait routine on.
     * Then pose 3, sound $2D, 6 frames, its motion stopped with gravity $30,
     * and it falls for good.
     */
    case ACTOR_START_SECOND:
        k->hit++;
        if (!k->low && !k->shots) {
            SoundQueueAdd(0x0F);
            AnimStart(a, 1);
            ActorSpeedYSetGravity(a, DIP_GRAVITY, DIP_SPEED);
            k->frames = 0x08;
            a->resume = SINK;
            return 1;
        }
        if (k->taken && g.health) {
            g.actors[0].resume = PLAYER_AT_FALL;
            k->taken = 0;
        }
        a->flags |= 0x80;
        return pose_and_fall(a);
    case SINK:
        ActorFallAgainst(a);
        if (--k->frames)
            return 1;
        return pose_and_fall(a);
    case FALL_SET:
        ActorMotionReset(a, 0x30);
        a->resume = FALL_AWAY;
        return 1;
    case FALL_AWAY:
        ActorFall(a);
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:F639 Kind79InRange
 * WHAT IT DOES: Keeps actor kind $79 near the point actors are kept in
 * range of: more than $D0 pixels to either side, its slot is cleared and
 * it ends; from $98 pixels below to $B0 above, nothing; out of that height
 * it starts again (state $0C with its low bits), and if it had been taken
 * up while Aladdin has health left, his code goes on at PlayerFall.
 */
static int Kind79InRange(Actor *a)
{
    if ((uint16_t)(g.range_x - pixels(a->x) + 0xD0 + view.dx) >= (uint16_t)(0x01A0 + 2 * view.dx)) {
        return ActorEndPlaced(a);
    }
    if ((uint16_t)(g.range_y - pixels(a->y) + 0x98) < 0x0148)
        return ACTOR_GOES_ON;
    if (k79(a)->taken && g.health)
        g.actors[0].resume = PLAYER_AT_FALL;
    a->state = (uint8_t)((a->state & 0x0F) | 0x0C);
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:F68C Kind79Wait
 * WHAT IT DOES: Kind $79's wait routine: its +$47 counted down; unless its
 * +$41 is set: state $8C when +$49 is set; taken up (+$42 up) when
 * Aladdin's test box's centre is in its zone $80 and he can take hold (no
 * state $8C counted, g.player_in_air set, health left, +$47 run out); a
 * shot that reaches it counted (+$43); state $8C when he lands on it; with
 * +$07 bit 1 and its y above $0032, state $8C and +$49 up. Then its
 * animation on, and Kind79InRange.
 */
int Kind79Wait(Actor *a)
{
    K79Own *k = k79(a);

    if (k->wait)
        k->wait--;
    if (!k->hit) {
        if (k->low)
            a->state |= 0x8C;
        if (ActorZoneHasTestPoint(a, TAKE_ZONE) && !g.player_hurt_count && g.player_in_air && g.health
            && !k->wait)
            k->taken++;
        if (ActorDownedByShot(a))
            k->shots++;
        if (PlayerLandsOnActor(a))
            a->state |= 0x8C;
        if ((a->spawn_byte & 0x02) && pixels(a->y) < 0x0032) {
            a->state |= 0x8C;
            k->low++;
        }
    }
    AnimStep(a);
    return Kind79InRange(a);
}

/*
 * 83:F6E9 Kind79EndIfCarry is dead: nothing in the game calls it (no JSR,
 * JSL, JMP, branch, pointer or word in the ROM names it; the record's
 * @unreachable 83:F6EB-F6F0, master 2026-11-19). Not converted.
 */
