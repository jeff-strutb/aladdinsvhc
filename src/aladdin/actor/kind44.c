/*
 * kind44.c: actor kind $44's start.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);

#define ACTOR_LOOK 0x2Bu        /* its look (word) */

/*
 * @routine 83:B45B Kind44Start
 * @actor
 * WHAT IT DOES: Actor kind $44: shows animation 4 for $20 frames, then
 * ends.
 * HOW: $08 bit 1 and $09 bits $90 set; its look ($2B) $0600; $14, $15 0;
 * $40 = $20; AnimStart 4; $20 frames (BRK first); JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
 * actor loop's jump for kind $44 in state $0C. Exit: never returns.
 */
void abi_Kind44Start(void)
{
    lda8(ea_dp(0x08));
    ora8_v(0x02);
    sta8(ea_dp(0x08));
    lda8(ea_dp(0x09));
    ora8_v(0x90);
    sta8(ea_dp(0x09));
    op_rep(0x20);
    lda16_v(0x0600);
    sta16(ea_dp(ACTOR_LOOK));
    op_sep(0x20);
    stz8(ea_dp(0x14));
    stz8(ea_dp(0x15));
    lda8_v(0x20);
    sta8(ea_dp(0x40));
    lda8_v(0x04);
    abi_AnimStart();
    do {
        actor_yield(0x83B47Eu);
        MEM8(dec, ea_dp(0x40));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}
