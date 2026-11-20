/*
 * wanted_c.c: stand-ins for the routines of other areas that kinds C's
 * files call (actor/kinds_c.h), until their owners' code lands. Each is
 * weak: the owner's definition replaces it at link time, and its line here
 * goes. Each prints its name once and does the least: nothing moves,
 * nothing is found, nothing ends.
 */
#include "actor/kinds_c.h"

#define STAND_IN __attribute__((weak))

/* n5-kinds-b, the shared actor code (actor.h) */
STAND_IN bool PlayerInReach(const Actor *a, uint8_t at) { (void)a; (void)at; unconverted("PlayerInReach"); return false; }

