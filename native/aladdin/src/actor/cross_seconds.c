/*
 * cross_seconds.c: second entries (state $80-$BF) that are another kind's
 * start: the record goes on in that kind's code from its start
 * (ActorRunAs), as the original's table jumps there. Each is a row's second
 * entry in kinds_b.c.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

/* 83:CFA9 Kind4EStart: kind $4D's second entry */
int Kind4DSecond(Actor *a) { return ActorRunAs(a, Kind4EStart, ACTOR_START); }

/* 83:F370 Kind71Start: kind $70's second entry */
int Kind70Second(Actor *a) { return ActorRunAs(a, Kind71Start, ACTOR_START); }

/* 84:9044 BackdropMover (kind $7C's start): kind $76's second entry (83:928A Kind76Second) */
int Kind76Second(Actor *a) { return ActorRunAs(a, Kind7CStart, ACTOR_START); }

/* 83:C96E PlatformCourseStart (kind $47's start): kind $62's second entry */
int Kind62Second(Actor *a) { return ActorRunAs(a, PlatformCourseStart, ACTOR_START); }

/* 83:F541 Kind79Rise (kind $79's start): kinds $77 and $78's second entry */
int Kind77Second(Actor *a) { return ActorRunAs(a, Kind79Rise, ACTOR_START); }
