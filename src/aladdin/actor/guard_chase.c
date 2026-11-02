/*
 * guard_chase.c: the bank $A5 guard's chase: keeping pace with the
 * player, and jumping up or down to a ledge when he is well above or
 * below.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorLedgeAbove(void);
void abi_ActorMotionPreset(void);
void abi_ActorProbeUpTowardPlayer(void);
void abi_GuardChase(void);
void abi_GuardLedgeJump(void);
void abi_PointInGroundLong(void);
void abi_RandomFromList(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"



