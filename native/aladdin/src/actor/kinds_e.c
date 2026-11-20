/*
 * kinds_e.c: the kinds n5-kinds-e converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-e's alone.
 *
 * Each row's entries are the ROM's tables' ($83:8CAB on; the notes'
 * kind-entries.txt lists every kind's): a start, second or third entry that
 * is a stub jumping to the kind's code is the row's code, and a wait stub
 * (kind_wait.c) is the row's wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"

const KindCode kinds_e[] = {
    /* $21: second entry 83:922F jumps to A5:F3EC, which no record routine
       holds and no script reaches: not converted (reported if reached) */
    { 0x21, Kind21Start, NULL, NULL, KindAnimateSend },
    /* $2E: its second entry is its wait routine (83:ABD8) entered as
       code, which no script reaches: not converted */
    { 0x2A, Kind2AStart, Kind2AStart, NULL, Kind2AWait },
    { 0x2E, Kind2EStart, NULL, NULL, ActorWaitEndLowKinds },
    { 0x31, Kind31Start, Kind31Start, NULL, Kind31Wait },
    { 0x35, Kind35Start, Kind35Start, NULL, ActorWaitAnimEnd10 },
    { 0x3B, Kind3BStart, Kind3BStart, NULL, Kind3BWait },
    { 0x54, Kind54Start, Kind54Start, NULL, ActorWaitHurtOnTouch },
    { 0x55, Kind55Start, Kind55Start, NULL, ActorWaitHurtOnTouch },
    { 0x98, Kind98Start, Kind98Start, NULL, Kind98WaitWork },
    { 0, NULL, NULL, NULL, NULL }
};
