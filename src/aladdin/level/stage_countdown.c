/*
 * stage_countdown.c: the countdown at $0343 set from the stage's entry of
 * its table as a stage starts.
 */
#include "machine.h"
#include "ops65.h"
void abi_StageCountdownSet(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 82:B563 StageCountdownSet
 * WHAT IT DOES: Sets the countdown at $0343 to the stage's value from the
 * table at $80:DB99.
 * HOW: The stage number ($033F) doubled indexes the table's words; the
 * word is copied a byte at a time.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: X = the stage
 * number * 2; A low byte = the word's high byte.
 */
void abi_StageCountdownSet(void)
{
    lda8(ea_abs(V_033F));  /* 82B563 lda */
    ACC8(asl);  /* 82B566 asl */
    tax8();  /* 82B567 tax */
    lda8(ea_absx(0xDB99));  /* 82B568 lda */
    sta8(ea_abs(V_0343));  /* 82B56B sta */
    lda8(ea_absx(0xDB9A));  /* 82B56E lda */
    sta8(ea_abs(0x0344));  /* 82B571 sta */
    return;  /* 82B574 rts */
}
