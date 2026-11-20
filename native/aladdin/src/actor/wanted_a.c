/*
 * wanted_a.c: stand-ins for the routines of other areas that actors A's
 * files call, until their owners' code lands. Each is weak: the owner's
 * definition replaces it at link time, and its line here goes. Each
 * prints its name once and does nothing.
 */
#include "actor/actor_own.h"
#include "player/player.h"
#include "video/video.h"

#define STAND_IN __attribute__((weak))

STAND_IN int PlayerEnter8CFacing(const Actor *by) { (void)by; unconverted("PlayerEnter8CFacing"); return 0; }
STAND_IN void ActorPictureSend(Actor *a) { (void)a; unconverted("ActorPictureSend"); }
