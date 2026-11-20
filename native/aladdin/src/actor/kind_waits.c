/*
 * kind_waits.c: the wait routines of several kinds: what each runs every
 * frame its actor waits (flags bit 7), after its turn and between. Each
 * returns ACTOR_GOES_ON, or ACTOR_ENDED when it ended its actor.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"

/* the own bytes these routines read, by record offset (+$3A on at 10) */
#define OWN_42 18
#define OWN_43 19

/*
 * @routine 83:A012 Kind27Wait
 * WHAT IT DOES: Kind $27's wait: animates; once its +$42 is set, touching
 * Aladdin while he has health it plays sound $4F, counts
 * g.figures_touched up and ends; ended too when out of range (box $58).
 * Its +$42 is set only by its start for $07 bit 3 (83:9E58).
 */
int Kind27Wait(Actor *a)
{
    AnimStep(a);
    if (!a->own.bytes[OWN_42])
        return ACTOR_GOES_ON;
    if (g.health && ActorHitsTestBox(a)) {
        SoundQueueAdd(0x4F);
        g.figures_touched++;
        return ActorEndPlaced(a);
    }
    return ActorEndOutOfRange(a, 0x58);
}

/*
 * @routine 83:B94A Kind30Wait
 * WHAT IT DOES: Kind $30's wait: animates; type 7 (bits 0-2 of $07) notes
 * the first shot that reaches it (+$43); ended when out of range (box
 * $18).
 */
int Kind30Wait(Actor *a)
{
    AnimStep(a);
    if ((a->spawn_byte & 0x07) == 0x07 && !a->own.bytes[OWN_43] && ActorTakesShot(a))
        a->own.bytes[OWN_43]++;
    return ActorEndOutOfRange(a, 0x18);
}
