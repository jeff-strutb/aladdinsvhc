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
    { 0x28, Kind28Start, Kind28Start, NULL, Kind28Wait },
    { 0x2A, Kind2AStart, Kind2AStart, NULL, Kind2AWait },
    { 0x2E, Kind2EStart, NULL, NULL, ActorWaitEndLowKinds },
    { 0x31, Kind31Start, Kind31Start, NULL, Kind31Wait },
    { 0x35, Kind35Start, Kind35Start, NULL, ActorWaitAnimEnd10 },
    { 0x3B, Kind3BStart, Kind3BStart, NULL, Kind3BWait },
    { 0x54, Kind54Start, Kind54Start, NULL, ActorWaitHurtOnTouch },
    { 0x55, Kind55Start, Kind55Start, NULL, ActorWaitHurtOnTouch },
    /* $97: its wait (83:93B9) is dead: nothing sets its +$09 bit 7 */
    /* $8F: its wait 83:FAAE KindAnimateSendFAAE is KindAnimateSend's code */
    /* $56: its second entry is its wait routine (83:D9B5) entered as code,
       which no script reaches: not converted; that wait is AnimStep only */
    { 0x56, Kind56Start, NULL, NULL, AnimateWhileWaiting },
    { 0x61, Kind61Start, Kind61Second, NULL, ActorWaitRange00 },
    { 0x64, Kind64Start, Kind64Second, NULL, ActorWaitRange00 },
    { 0x65, Kind65Start, NULL, NULL, AnimateWhileWaiting },
    { 0x8F, Kind8FWalk, Kind8FWalk, NULL, KindAnimateSend },
    /* $94: its wait 83:FB15 Kind94Wait is AnimateWhileWaiting's code */
    { 0x94, Kind94Spark, Kind94Second, NULL, AnimateWhileWaiting },
    { 0x40, Kind40Start, Kind40Start, NULL, Kind40Wait },
    { 0x7B, Kind7BStart, Kind7BStart, NULL, Kind7BWait },
    { 0xA8, KindA8Start, NULL, NULL, NULL },
    /* $48's wait 83:CE25 Kind48Wait is AnimStep only: AnimateWhileWaiting */
    { 0x48, Kind48Start, Kind48Start, NULL, AnimateWhileWaiting },
    { 0x58, Kind58Start, NULL, NULL, Kind58Wait },
    { 0x5A, Kind5AStart, NULL, NULL, Kind5AWait },
    { 0x73, Kind73Start, Kind73Start, NULL, Kind73Work },
    { 0x97, Kind97Start, NULL, NULL, NULL },
    { 0x98, Kind98Start, Kind98Start, NULL, Kind98WaitWork },
    { 0x9E, Kind9EStart, NULL, NULL, Kind9EWaitWork },
    { 0xA1, KindA1Start, KindA1Start, NULL, KindA1Touch },
    { 0xA6, KindA6Stay, NULL, NULL, NULL },
    { 0, NULL, NULL, NULL, NULL }
};
