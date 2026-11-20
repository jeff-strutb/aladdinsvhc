/*
 * wanted_c.c: stand-ins for the routines of other areas that kinds C's
 * files call (actor/kinds_c.h), until their owners' code lands. Each is
 * weak: the owner's definition replaces it at link time, and its line here
 * goes. Each prints its name once and does the least: nothing moves,
 * nothing is found, nothing ends.
 */
#include "actor/kinds_c.h"
#include "map/map.h"

#define STAND_IN __attribute__((weak))

/* n5-actors-a (actor.h) */
STAND_IN uint8_t ActorMapAhead5Down12(Actor *a) { (void)a; unconverted("ActorMapAhead5Down12"); return 1; }
STAND_IN void ActorVariantChildStep(Actor *a) { (void)a; unconverted("ActorVariantChildStep"); }
STAND_IN void ActorSpeedXSetAccel(Actor *a, uint8_t c, uint8_t at) { (void)a; (void)c; (void)at; unconverted("ActorSpeedXSetAccel"); }
STAND_IN void ActorMoveAlong(Actor *a, uint16_t d, uint8_t v) { (void)a; (void)d; (void)v; unconverted("ActorMoveAlong"); }

/* native-video (video.h) */
STAND_IN void ActorPictureQueue(Actor *a) { (void)a; unconverted("ActorPictureQueue"); }

/* n5-levels (map.h) */
STAND_IN int ActorOutOfWallProbe4At(Actor *a, MapPoint *p) { (void)a; p->behaviour = 0; unconverted("ActorOutOfWallProbe4At"); return 0; }
