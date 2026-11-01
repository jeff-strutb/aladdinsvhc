/*
 * anim_pick.c: choosing which animation an actor starts.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);

/* In the bank DB names: an animation by the actor's index at $40. */
#define ANIM_BY_40 0xD714u

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/*
 * @routine 84:8A48 ActorAnimBy40
 * WHAT IT DOES: Starts the animation the actor's index at $40 picks.
 * HOW: The index selects a byte of the table at $D714; AnimStart.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB the table's bank. Exit: as
 * AnimStart.
 */
void abi_ActorAnimBy40(void)
{
    uint8_t x = rd8(dp(ACTOR_40)), a;

    cpu.x = x;
    a = rd8(in_db(ANIM_BY_40 + x));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    abi_AnimStart();
}

/*
 * @routine A5:EA15 ActorAnimChange
 * WHAT IT DOES: Starts animation A unless it is the one the actor already
 * plays.
 * HOW: A doubled is compared with the actor's animation offset; AnimStart
 * when they differ.
 * Entry: M 8-bit, X 8-bit; A = the animation; D = the actor. Exit: the
 * same: A low = twice the animation, N, Z and C from the compare (Z set).
 * Else as AnimStart.
 */
void abi_ActorAnimChange(void)
{
    uint8_t a = (uint8_t)cpu.a, d;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    d = (uint8_t)(a << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | d);
    cpu.p = cmp8_flags(cpu.p, d, rd8(dp(ACTOR_ANIM)));
    if (cpu.p & P_Z)
        return;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (d & 1));
    d = (uint8_t)(d >> 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | d);
    cpu.p = nz8(cpu.p, d);
    abi_AnimStart();
}
