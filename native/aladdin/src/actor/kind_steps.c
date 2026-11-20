/*
 * kind_steps.c: what kinds that only animate, send their own pictures or
 * go once out of range do each frame they wait; and the steps of an actor
 * along a path.
 *
 * Four of the record's routines here do only AnimStep and are the actor
 * core's AnimateWhileWaiting (kinds.c), which the rows of their kinds name:
 *   @routine 84:9533 KindAnimate9533 (kind $82's wait)
 *   @routine 84:9628 KindAnimate9628 (kind $85's wait and second entry's work)
 *   @routine 84:9EF0 KindAnimate9EF0 (kind $95's wait)
 *   @routine 84:9F4A KindAnimate9F4A (kind $96's wait)
 * and three more the same, for the rows of kinds $24, $A3 and $9C:
 *   @routine 84:8048 Kind24Animate
 *   @routine 84:8399 KindA3Animate
 *   @routine 84:A81B Kind9CAnimate
 * Two more are KindAnimateSend's code exactly, for the rows of kinds $21
 * and $93:
 *   @routine A5:F3E3 Kind21AnimateSend
 *   @routine A5:F434 Kind93AnimateSend
 * Two more are each one kind's wait, so its kind's owner writes them (one
 * kind, one owner): 84:9387 KindAnimateFreeOff (kind $7F, kinds C) and
 * 84:A975 KindA0AnimateFreeOff (kind $A0, kinds D). The path steps
 * (84:8CAF ActorPathStep, 84:8DFB ActorPathNext) come with kind $73.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

/*
 * @routine 84:903B KindAnimateSend
 * WHAT IT DOES: One frame of a kind that animates its own picture: its
 * animation on, then its picture sent (ActorPictureSend). Kind $76's wait, and (as Kind21AnimateSend and
 * Kind93AnimateSend) kinds $21 and $93's.
 */
int KindAnimateSend(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    return ACTOR_GOES_ON;
}




/*
 * @routine A5:F603 Kind91FreeOff
 * WHAT IT DOES: What actors of kinds $91 and $92 do each frame they wait
 * (and the work of their second entries): end once out of range (box 0).
 * Shared by two kinds, so it stays with its record file's owner.
 */
int Kind91FreeOff(Actor *a)
{
    return ActorFreeOutOfRange(a, 0x00) ? ACTOR_ENDED : ACTOR_GOES_ON;
}

/* a path's own bytes (OwnPlace places; +$4A-$4B are Actor.stream) */
#define PATH_AT(a) ((uint16_t)((a)->own.bytes[OWN_42] | (a)->own.bytes[OWN_43] << 8))    /* +$42: its place along it (word) */
#define PATH_FIRST(a) ((a)->own.bytes[OWN_45])    /* +$45: frames each step of a new run is taken for */
#define PATH_EACH(a) ((a)->own.bytes[OWN_46])     /* +$46: frames each step is taken for */
#define PATH_FRAMES(a) ((a)->own.bytes[OWN_47])   /* +$47: frames left of this step; 0: stopped */
#define PATH_STEP(a) ((uint16_t)((a)->own.bytes[OWN_48] | (a)->own.bytes[OWN_49] << 8))  /* +$48: the step added each frame (word) */
#define PATH_ON(a) ((a)->own.bytes[OWN_4C])       /* +$4C: nonzero: it follows the path */
#define PATH_STEPS(a) ((a)->own.bytes[OWN_4D])    /* +$4D: steps left of this run */

/*
 * @routine 84:8DFB ActorPathNext
 * WHAT IT DOES: Counts down the path's step; at its end takes the next
 * step (the word at its place in the list, turned by the speed's low word
 * and plus the word at +$1C: accel x and the speed down's fraction), the
 * frames reloaded; the run's steps count down; at 0 the next run's list
 * and its step count from the list, or, a zero list, the path is done
 * (+$18 = 1).
 */
void ActorPathNext(Actor *a)
{
    uint16_t v;

    if (!PATH_FRAMES(a) || --PATH_FRAMES(a))
        return;
    v = (uint16_t)((path_word(a->stream, 0) ^ (uint16_t)a->speed_x) + (uint16_t)(a->accel_x | (a->speed_y & 0xFF) << 8));
    a->own.bytes[OWN_48] = (uint8_t)v;
    a->own.bytes[OWN_49] = (uint8_t)(v >> 8);
    PATH_FRAMES(a) = PATH_EACH(a);
    if (--PATH_STEPS(a)) {
        a->stream.at += 2;
        return;
    }
    PATH_EACH(a) = PATH_FIRST(a);
    {
        uint16_t next = path_word(a->stream, 2);

        if (!next) {
            PATH_FRAMES(a) = 0;
            a->probe_at = 1;                    /* +$18: idle, for the list walk at 84:8A52 */
            return;
        }
        PATH_STEPS(a) = path_byte(a->stream, 4);
        a->stream = path_at(a->stream, next);
    }
}

/*
 * @routine 84:8CAF ActorPathStep
 * WHAT IT DOES: One step of the actor along its path: its parity bits set,
 * its place along it (+$42) moved by the step, and the path goes on
 * (ActorPathNext) when it is on.
 */
void ActorPathStep(Actor *a)
{
    uint16_t at;

    ActorParityBitsSet(a);
    at = (uint16_t)(PATH_AT(a) + PATH_STEP(a));
    a->own.bytes[OWN_42] = (uint8_t)at;
    a->own.bytes[OWN_43] = (uint8_t)(at >> 8);
    if (PATH_ON(a))
        ActorPathNext(a);
}
