/*
 * kinds_player.c: the kinds n5-player converts, registered for the actor loop
 * (kinds.c searches this table). One row a kind: { kind, its code, its code
 * again if it has a second entry, again if a third, its wait routine }
 * (actor_own.h's KindCode; actor-core.md section 3); the last row's code
 * and wait NULL. This file is n5-player's alone.
 */
#include "actor/actor_own.h"
#include "player/player.h"

const KindCode kinds_player[] = {
    /* Aladdin (player/player_code.c): start 83:91F4 Kind00Start, second
       83:91FD Kind00Second (kind $01: 83:9201 Kind01Second), wait 83:91F8
       ActorKind00Wait; the record's tables give these kinds no third entry */
    { 0x00, PlayerCode, PlayerCode, NULL, PlayerWait },
    { 0x01, PlayerCode, PlayerCode, NULL, PlayerWait },
    { 0x02, PlayerCode, PlayerCode, NULL, PlayerWait },
    { 0x03, PlayerCode, PlayerCode, NULL, PlayerWait },
    { 0x07, PlayerCode, PlayerCode, NULL, PlayerWait },
    /* the ground-push mark: 83:E480 GroundPushMark, wait 83:E4E7 ActorWaitAnimB (= AnimateWhileWaiting) */
    { 0x60, Kind60Start, NULL, NULL, AnimateWhileWaiting },
    { 0, NULL, NULL, NULL, NULL }
};
