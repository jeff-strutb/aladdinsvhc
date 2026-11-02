/*
 * player_touch.c: what an actor does when Aladdin touches it: hurts him,
 * or, when he comes down on it from above, he bounces off it (and may
 * stomp it).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_PlayerEnter8CFacing(void);
void abi_ScratchBelowBy10(void);

#define ACTOR_11 0x11u              /* taken one from as the player stomps it */
#define HEALTH 0x0367u              /* the player's health */
#define PLAYER_SPEED_Y_WHOLE (PLAYER + ACTOR_SPEED_Y + 1) /* negative: he is going up (word) */
#define V_141B 0x141Bu              /* nonzero: no bounce */
#define V_141C 0x141Cu              /* zero: no bounce */
#define V_145A 0x145Au              /* negative: no bounce */
#define V_1448 0x1448u              /* $3C: the bouncing actor is not touched */
#define STOMPED 0x8Cu               /* the state a stomped actor goes into */

/* Not yet filed */
#define PLAYER_BOUNCE 0x8384D5u     /* the player bounces up off the actor */
#define STOMP_MARK 0x8384F4u        /* an actor of kind $11 started at the player's place */
#define TOUCH_8362 0x838362u        /* a touch test (ActorHitsTestBoxIfActive first) */
#define TOUCH_844A 0x83844Au        /* a touch test (ActorHitsTestBoxIfActive first) */

/*
 * @routine 83:80A1 ActorHurtsPlayerOnTouchLong
 * WHAT IT DOES: ActorHurtsPlayerOnTouch for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorHurtsPlayerOnTouch; called by JSL. Exit: as
 * ActorHurtsPlayerOnTouch.
 */
void abi_ActorHurtsPlayerOnTouchLong(void)
{
    abi_ActorHurtsPlayerOnTouch();
}









