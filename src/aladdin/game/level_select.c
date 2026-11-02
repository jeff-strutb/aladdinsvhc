/*
 * level_select.c: the level select (a development mode): L and R held
 * with a direction step the level part and stage, and the level text
 * shows them.
 */
#include "machine.h"
#include "ops65.h"
void abi_LevelNumberText(void);
void abi_LevelSelect(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"


