/*
 * kinds.c: each actor kind's code, by kind number, in place of the
 * original's tables of entries by kind ($83:8CAB and $83:8DAB the starts,
 * $83:8DFD and $83:8EFD the second entries, $83:90A1 and $83:91A1 the third,
 * $83:8F4F and $83:904F the wait routines). A kind's code is one function
 * for all its entries; it goes on from where it stopped by its record's
 * resume point. A kind or entry not converted yet prints its number once.
 */
#include <stdio.h>

#include "actor/actor_own.h"

typedef struct {
    uint8_t kind;
    ActorCode start;            /* its start entry (state $0C-$7F) */
    ActorCode second;           /* its second (state $80-$BF), NULL none converted */
    ActorWaitCode wait;         /* its wait routine */
} KindCode;

static const KindCode kinds[] = {
    { 0x36, Kind36Start, NULL, AnimateWhileWaitingIf07Bit4 },   /* second entry $83:B0C7: not converted */
    { 0x4A, Kind4AShow, NULL, AnimateWhileWaiting },            /* second entry $83:B4B9: not converted */
    { 0x82, Kind82Show, NULL, AnimateWhileWaiting },
    { 0x85, Kind85Start, NULL, AnimateWhileWaiting },           /* second entry 83:92FB Kind85Second: not converted */
    { 0x95, Kind95Start, NULL, AnimateWhileWaiting },
    { 0x96, SteppedShowActor, NULL, AnimateWhileWaiting },
};

static const KindCode *find(uint8_t kind)
{
    unsigned i;

    for (i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
        if (kinds[i].kind == kind)
            return &kinds[i];
    return NULL;
}

ActorCode actor_kind_code(uint8_t kind, uint8_t entry)
{
    static uint8_t told[256];
    const KindCode *k = find(kind);
    ActorCode code = NULL;

    if (k)
        code = entry == ACTOR_START_SECOND ? k->second : entry == ACTOR_START_THIRD ? NULL : k->start;
    if (!code && !(told[kind] & 1u << (entry & 3))) {
        told[kind] |= (uint8_t)(1u << (entry & 3));
        fprintf(stderr, "actor kind $%02X: %s not converted\n", kind,
                entry == ACTOR_START_SECOND ? "its second entry" : entry == ACTOR_START_THIRD ? "its third entry" : "its code");
    }
    return code;
}

ActorWaitCode actor_kind_wait(uint8_t kind)
{
    static uint8_t told[256];
    const KindCode *k = find(kind);

    if (!k && !told[kind]++)
        fprintf(stderr, "actor kind $%02X: its wait routine not converted\n", kind);
    return k ? k->wait : NULL;
}

/*
 * @routine 83:B4B9 ActorWaitAnim
 * WHAT IT DOES: Each frame it waits, its animation plays on (AnimStep).
 * The wait routine of kinds $4A, and (through 84:9533, 84:9628, 84:9EF0,
 * 84:9F4A KindAnimate*) of $82, $85, $95 and $96.
 */
int AnimateWhileWaiting(Actor *a)
{
    AnimStep(a);
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:B0BB ActorWaitAnimIf07Bit4
 * WHAT IT DOES: Each frame it waits, its animation plays on when bit 4 of
 * its +$07 is set. Kind $36's wait routine.
 */
int AnimateWhileWaitingIf07Bit4(Actor *a)
{
    if (a->spawn_byte & 0x10)
        AnimStep(a);
    return ACTOR_GOES_ON;
}
