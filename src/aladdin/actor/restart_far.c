/*
 * restart_far.c: an actor of bank $A5 sent back to its restart point
 * ($4E) once the player is far to its left.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorRestartIfPlayerFarLeft(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine A5:EF83 ActorRestartIfPlayerFarLeft
 * WHAT IT DOES: When the player is more than 192 pixels left of the actor,
 * returns to the actor's restart point ($4E) instead of to the caller.
 * HOW: The player's x + $C0 (+ C at entry) against the actor's x: lower,
 * the return address pulled and $4E - 1 pushed in its place, so the RTS
 * goes to $4E.
 * Entry: C added in; D = the actor; called by JSR. Exit: M 8-bit; C
 * clear when restarting, A = $4E - 1; else C set, A = the sum.
 */
void abi_ActorRestartIfPlayerFarLeft(void)
{
    op_rep(0x20);  /* A5EF83 rep */
    lda16(ea_abs(0x08E7));  /* A5EF85 lda */
    adc16_v(0x00C0);  /* A5EF88 adc */
    cmp16_v(rd16(ea_dp(0x0C)));  /* A5EF8B cmp */
    if (IF_C) goto L_A5EF94;  /* A5EF8D bcs */
    pla16();  /* A5EF8F pla */
    lda16(ea_dp(0x4E));  /* A5EF90 lda */
    ACC16(dec);  /* A5EF92 dec */
    pha16();  /* A5EF93 pha */
L_A5EF94:
    op_sep(0x20);  /* A5EF94 sep */
    return;  /* A5EF96 rts */
}
