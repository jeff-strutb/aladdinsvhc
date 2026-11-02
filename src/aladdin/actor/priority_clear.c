/*
 * priority_clear.c: an actor's sprite priority bits cleared, except in
 * the level that keeps them.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorPriorityClear(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:81B2 ActorPriorityClear
 * WHAT IT DOES: Clears the actor's sprite priority bits, except in level
 * $0E.
 * HOW: Unless the level's low byte is $0E, bits 4-5 of $09 cleared (TRB
 * $30).
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: A low byte = $30 or the level; Z from the TRB or
 * the compare.
 */
void abi_ActorPriorityClear(void)
{
    lda8(ea_abs(LEVEL));  /* 8481B2 lda */
    cmp8_v(0x0E);  /* 8481B5 cmp */
    if (IF_Z) goto L_8481BD;  /* 8481B7 beq */
    lda8_v(0x30);  /* 8481B9 lda */
    trb8(ea_dp(0x09));  /* 8481BB trb */
L_8481BD:
    return;  /* 8481BD rts */
}
