/*
 * kind_5e.c: actor kind $5E: a brief show of its animation, then its end.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);

#define KIND5E_LEFT 0x40u  /* frames left */

/*
 * @routine 83:E42A Kind5EBrief
 * @actor
 * WHAT IT DOES: Actor kind $5E: shows animation $0C for $23 frames, then
 * ends (its $07 bit 2 picks the same either way).
 * HOW: Its look $0600; $08 bits 0-1 and $09 bits $90 set; animation $0C
 * (X) and $40 = $23 (Y), the same whether bit 2 of $07 is set; AnimStart;
 * $23 frames (BRK first, $40 down); then the jump at $83:E461 into
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $5E in state $0C. Exit: never returns;
 * ends in ActorEndPlaced.
 */
void abi_Kind5EBrief(void)
{
    uint8_t left;

    dp16_set(ACTOR_LOOK, 0x0600);
    cpu.a = 0x0600;
    a8_load(0x03);
    tsb_at(dpa(ACTOR_08), 0x03);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    /* animation $0C for $23 frames (both of its $07 variants alike) */
    x8_load(0x0C);
    y8_load(0x23);
    a8_load(rd8(dpa(ACTOR_07)) & 0x04);
    a8_load(0x0C);
    wr8(dpa(KIND5E_LEFT), 0x23);
    abi_AnimStart();
    do {
        actor_yield(0x83E450u);
        left = (uint8_t)(rd8(dpa(KIND5E_LEFT)) - 1);
        wr8(dpa(KIND5E_LEFT), left);
        nz8_of(left);
    } while (left);
    abi_ActorEndPlaced();
}
