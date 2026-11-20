/*
 * chain_piece.c: the pieces of the nine-piece chain (kinds $74 and $75) and its head (kind $73).
 */
#include "actor/actor_own.h"

/*
 * @routine 84:8ADE ChainPieceEnd
 * WHAT IT DOES: A piece of the chain (or its head) goes back to what it
 * was doing before it was struck: its resume point from its own +$35-$36
 * (kept when it was struck), anim_flags bit 4 and flags bit 7 set; it goes
 * on at its next turn (the original leaves for the next record: ACTOR_LOOP_NEXT).
 */
int ChainPieceEnd(Actor *a)
{
    a->resume = (uint16_t)(a->own.bytes[OWN_35] | a->own.bytes[OWN_36] << 8);
    a->anim_flags |= 0x10;
    a->flags |= 0x80;
    return 1;
}
