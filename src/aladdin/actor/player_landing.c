/*
 * player_landing.c: Aladdin coming down on top of an actor: he is stood on
 * it, his code goes on at his landing, and an actor of kind $11 starts at
 * his feet.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ScratchBelowBy10(void);

#define V_1448 0x001448u           /* $3C: no landing on actors */
#define V_0367 0x000367u           /* counted down on entering state $8C; 0: no landing */
#define PLAYER_STATE_8C_COUNT 0x00141Bu /* counted up on entering state $8C */
#define V_141C 0x00141Cu           /* 0: no landing */
#define V_145A 0x00145Au           /* bit 7 set: no landing */
#define PLAYER_SPEED_Y_PIXELS (PLAYER + ACTOR_SPEED_Y + 1) /* his vertical speed's whole part (word) */
#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
/* Not yet filed: Aladdin's code to go on at $D77C of his bank, and him stood
   on the actor's top (its Y less the boxes' half-heights). */
#define PLAYER_STAND_ON_ACTOR 0x8384D5u
#define ACTOR_TAKE_B 0x81EA3Du     /* a record from the second free list: X, N set when none */
#define KIND_11 0x11


