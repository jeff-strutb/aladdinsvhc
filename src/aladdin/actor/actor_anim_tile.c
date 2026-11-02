/*
 * actor_anim_tile.c: a bank $A5 actor's animation chosen by the tile it
 * stands on and its speed.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorAnimByTile(void);
void abi_ActorAnimChange(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

