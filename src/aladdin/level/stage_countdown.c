/*
 * stage_countdown.c: the countdown at $0343 set from the stage's entry of
 * its table as a stage starts.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_StageCountdownSet(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define STAGE_COUNTDOWNS 0xDB99u  /* by stage, its countdown (words, in the bank DB names) */

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
    uint8_t stage = rd8(dba(V_033F));
    uint8_t at = (uint8_t)(stage << 1);
    uint16_t count = rd16(dba(STAGE_COUNTDOWNS + at));

    x8_load(at);
    flag_set(P_C, stage & 0x80);
    wr16(dba(V_0343), count);
    a8_load((uint8_t)(count >> 8));
}
