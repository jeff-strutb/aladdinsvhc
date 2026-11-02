/*
 * camera_trigger.c: a level's task (created at $80:8943) that waits for
 * the camera to reach two places: at the first it frees the actors, at
 * the second (the player alive) it sets the player and the escort going
 * again and starts an actor of kind $6B.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorSpawnAt(void);
void abi_ActorsFreeAll(void);
void abi_CameraTriggerTask(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

