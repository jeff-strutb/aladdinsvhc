/*
 * spawn_every8.c: a child the player starts every eighth frame. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorSpawnFacing(void);

#define V_032D 0x032Du  /* the high byte of the word at $032C: a spawn when its low 3 bits are 0 */

/*
 * @routine 81:DD90 PlayerSpawnEvery8
 * WHAT IT DOES: Every eighth frame (low three bits of $032D 0) starts a
 * child of hit box $34 at the player's spawn point 2, facing as the player
 * faces.
 * HOW: Spawn point 2; the flip kept on the stack and set to the facing;
 * ActorSpawnFacing with hit box $34 and Y = 1; the flip put back. No
 * record free: X 8-bit and the kept flip pulled, not put back.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:D28D). Exit: M 8-bit; started: as
 * ActorSpawnFacing, the flip put back; none free: X 8-bit, the player's
 * flip left as its facing; not this frame: A low nonzero.
 */
void abi_PlayerSpawnEvery8(void)
{
    uint8_t phase = rd8(dba(V_032D)) & 0x07;

    a8_load(phase);
    if (phase)
        return;
    /* kind $34 from spawn point 2, facing his way */
    a8_load(0x02);
    wr8(dpa(ACTOR_SPAWN_AT), 0x02);
    a8_load(rd8(dpa(ACTOR_FLIP)));
    pha8();                               /* his flip, kept */
    x8_load(rd8(dpa(ACTOR_FACING)));
    wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.x);
    a8_load(0x34);
    y8_load(0x01);
    abi_ActorSpawnFacing();
    if (cpu.p & P_N) {                    /* none free: the flip stays */
        op_sep(P_X);
        pla8();
        return;
    }
    pla8();
    wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
}

