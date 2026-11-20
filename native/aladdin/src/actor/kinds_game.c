/*
 * kinds_game.c: the kinds the game-screens session converts (game/, level/,
 * map/, system/, input/: $26 game/continue_choice.c, and $9D if it is
 * theirs), registered for the actor loop (kinds.c searches this table).
 * One row a kind: { kind, its code, its code again if it has a second
 * entry, again if a third, its wait routine } (actor_own.h's KindCode;
 * actor-core.md section 3); the last row's code and wait NULL. This file is
 * the game-screens session's alone.
 */
#include "actor/actor_own.h"
#include "game/game.h"

const KindCode kinds_game[] = {
    { 0x26, ContinueChoiceStart, NULL, NULL, ActorWaitHoldPlayer },
    { 0, NULL, NULL, NULL, NULL }
};
