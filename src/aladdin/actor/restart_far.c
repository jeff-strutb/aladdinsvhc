/*
 * restart_far.c: an actor of bank $A5 sent back to its restart point
 * ($4E) once the player is far to its left.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorRestartIfPlayerFarLeft(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

