/*
 * kinds.c: each actor kind's code, by kind number, in place of the
 * original's tables of entries by kind ($83:8CAB and $83:8DAB the starts,
 * $83:8DFD and $83:8EFD the second entries, $83:90A1 and $83:91A1 the third,
 * $83:8F4F and $83:904F the wait routines). An actor starts in its entry's
 * code and goes on in the code that owns its resume point (Actor.code). A
 * kind or entry not converted yet prints its number once.
 */
#include <stdio.h>
#include <stdlib.h>

#include "actor/actor_own.h"

/* this file's kinds (the actor core's); the other sessions' are in their own
   files, searched in turn */
const KindCode kinds_core[] = {
    { 0x36, Kind36Start, NULL, NULL, AnimateWhileWaitingIf07Bit4 },   /* second entry $83:B0C7: not converted */
    { 0x4A, Kind4AShow, NULL, NULL, AnimateWhileWaiting },            /* second entry $83:B4B9: not converted */
    { 0x82, Kind82Show, NULL, NULL, AnimateWhileWaiting },
    { 0x85, Kind85Start, NULL, NULL, AnimateWhileWaiting },           /* second entry 83:92FB Kind85Second: not converted */
    { 0x95, Kind95Start, NULL, NULL, AnimateWhileWaiting },
    { 0x96, SteppedShowActor, NULL, NULL, AnimateWhileWaiting },
    { 0, NULL, NULL, NULL, NULL }
};

static const KindCode *const tables[] = { kinds_core, kinds_a, kinds_b, kinds_c, kinds_d, kinds_e, kinds_player };
static const char *const table_names[] = { "kinds.c", "kinds_a.c", "kinds_b.c", "kinds_c.c", "kinds_d.c", "kinds_e.c",
                                            "kinds_player.c" };

/* Every table's rows, once: a kind in two rows ends the run with both
   tables named. One row a kind; a function may serve several kinds. */
static void tables_check(void)
{
    const char *where[256] = { 0 };
    unsigned t, faults = 0;
    const KindCode *k;

    for (t = 0; t < sizeof tables / sizeof tables[0]; t++)
        for (k = tables[t]; k->start || k->wait; k++) {
            if (where[k->kind]) {
                fprintf(stderr, "actor kinds: kind $%02X is registered in %s and in %s\n", k->kind, where[k->kind],
                        table_names[t]);
                faults++;
            }
            where[k->kind] = table_names[t];
        }
    if (faults)
        exit(1);
}

static const KindCode *find(uint8_t kind)
{
    static int checked;
    unsigned t;
    const KindCode *k;

    if (!checked++)
        tables_check();
    for (t = 0; t < sizeof tables / sizeof tables[0]; t++)
        for (k = tables[t]; k->start || k->wait; k++)
            if (k->kind == kind)
                return k;
    return NULL;
}

ActorCode actor_kind_code(uint8_t kind, uint8_t entry)
{
    static uint8_t told[256];
    const KindCode *k = find(kind);
    ActorCode code = NULL;

    if (k)
        code = entry == ACTOR_START_SECOND ? k->second : entry == ACTOR_START_THIRD ? k->third : k->start;
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
