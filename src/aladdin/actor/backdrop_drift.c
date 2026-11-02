/*
 * backdrop_drift.c: the actor of kind $67 (its start at $83:E5F1) moves
 * the backdrop layer up and down the screen, by its speed a frame, in
 * turns. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

/* The backdrop's height against the play field's, added to the play
   field's scroll Y for the backdrop's ($81:CA6A): 32-bit, a fraction byte
   first. */
#define BACK_Y_OFFSET 0x001A2Bu
#define DRIFT_SPEED 0x42u          /* the actor's: fraction pixels a frame (word) */

/*
 * @routine 83:E653 BackdropDriftDown
 * WHAT IT DOES: Moves the backdrop down the screen by the actor's speed:
 * its offset against the play field's scroll falls.
 * HOW: A 32-bit subtraction of the speed from the offset, a word at a
 * time.
 * Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A = the offset's top word; N, V, Z and C from its
 * subtraction. M 16-bit; X and Y unchanged.
 */
void abi_BackdropDriftDown(void)
{
    sec();
    lda16(ea_abs(BACK_Y_OFFSET));
    sbc16_v(rd16(ea_dp(DRIFT_SPEED)));
    sta16(ea_abs(BACK_Y_OFFSET));
    lda16(ea_abs(BACK_Y_OFFSET + 2));
    sbc16_v(0x0000);
    sta16(ea_abs(BACK_Y_OFFSET + 2));
}

/*
 * @routine 83:E666 BackdropDriftUp
 * WHAT IT DOES: Moves the backdrop up the screen by the actor's speed:
 * its offset against the play field's scroll grows.
 * HOW: A 32-bit addition of the speed to the offset, a word at a time.
 * Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A = the offset's top word; N, V, Z and C from its
 * addition. M 16-bit; X and Y unchanged.
 */
void abi_BackdropDriftUp(void)
{
    clc();
    lda16(ea_abs(BACK_Y_OFFSET));
    adc16_v(rd16(ea_dp(DRIFT_SPEED)));
    sta16(ea_abs(BACK_Y_OFFSET));
    lda16(ea_abs(BACK_Y_OFFSET + 2));
    adc16_v(0x0000);
    sta16(ea_abs(BACK_Y_OFFSET + 2));
}
