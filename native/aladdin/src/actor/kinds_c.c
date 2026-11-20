/*
 * kinds_c.c: the kinds n5-kinds-c converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-kinds-c's alone.
 */
#include "actor/kinds_c.h"

const KindCode kinds_c[] = {
    { 0x23, Kind23Code, Kind23Code, NULL, Kind23Wait },
    { 0x25, Kind25Code, NULL, NULL, ActorWaitNothing },
    { 0x2B, Kind2BCode, Kind2BCode, NULL, Kind2BWait },
    { 0x2C, Kind2CCode, Kind2CCode, NULL, Kind2CWait },
    { 0x2D, Kind2DCode, Kind2DCode, Kind2DCode, ActorWaitHitsRange8 },
    { 0x2F, Kind2FCode, Kind2FCode, NULL, ActorWaitHitsRange0 },
    { 0x33, Kind33Code, NULL, NULL, Kind33Wait },              /* its second (83:BB96) is an RTS no state reaches */
    { 0x3A, Kind3ACode, Kind3ACode, NULL, Kind33Wait },        /* its wait 83:BF7E is Kind33Wait's bytes */
    { 0x3E, Kind3ECode, Kind3ECode, NULL, Kind3EWait },
    { 0x3D, Kind3DCode, Kind3DCode, NULL, Kind3DWait },
    { 0x41, Kind41Code, Kind41Code, NULL, Kind41Wait },
    { 0x4F, Kind4FCode, NULL, NULL, ActorWaitTest42Anim2 },    /* its second (83:D108) is an RTS no state reaches */
    { 0x50, Kind50Code, Kind50Code, NULL, Kind50Wait },
    { 0x59, Kind59Code, NULL, NULL, Kind23Wait },              /* its wait 83:DEB9 is Kind23Wait's bytes */
    { 0x63, Kind63Code, Kind63Code, Kind63Code, Kind63Wait },
    { 0x6B, Kind6BCode, NULL, NULL, AnimateWhileWaiting },     /* its wait 83:ECCF is AnimStep and an RTS */
    { 0x7E, Kind7ECode, Kind7ECode, NULL, Kind7EWait },        /* its second entry is kind $7F's start: the same code here */
    { 0x7F, Kind7FCode, Kind7FCode, NULL, Kind7FWait },
    { 0x81, Kind81Code, Kind81Code, NULL, NULL },              /* its wait 84:951C does nothing */
    { 0x83, Kind83Code, NULL, NULL, Kind83Wait },              /* its second entry is its wait routine's code: no state reaches it */
    { 0x8D, Kind8DCode, Kind8DCode, NULL, Kind8DWait },
    { 0x8E, Kind8ECode, NULL, NULL, NULL },
    { 0x90, Kind90Code, NULL, NULL, NULL },                    /* its second and its wait (83:9366) reach an RTL: nothing */
    { 0, NULL, NULL, NULL, NULL }
};
