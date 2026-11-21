/*
 * kind92.c: actor kind $92, a marker that puts the level's escort actor at its own place and clears
 * the escort's mode, then ends. Its wait is Kind91FreeOff (kind_steps.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);

#define K92_ESCORT_RESUME 0xE826u     /* where the escort's code resumes (its word at +3) */
#define ESCORT_X_PIXELS (ACTOR_ESCORT + ACTOR_X + 1)   /* the escort's x (pixels, word) */
#define ESCORT_Y_PIXELS (ACTOR_ESCORT + ACTOR_Y + 1)   /* the escort's y (pixels, word) */
#define ESCORT_RESUME (ACTOR_ESCORT + ACTOR_03)        /* where its code resumes (word) */
#define ESCORT_47 (ACTOR_ESCORT + 0x47)                /* the escort's byte its marker's $07 is kept in */
#define ESCORT_BUSY (ACTOR_ESCORT + 0x45)              /* the escort's: nonzero while it is busy */
#define ESCORT_MODE (ACTOR_ESCORT + 0x4D)              /* the escort's mode */
#define K92_WAYS 0xF5DCu              /* (bank $A5) its ways by $07 (words) */

/*
 * @routine A5:F5BA Kind92Start
 * @actor
 * WHAT IT DOES: Actor kind $92, a marker that puts the level's escort
 * actor at its own place, to resume at $E826, with its $07 in the
 * escort's $47, and clears the escort's mode (and, for $07 0, its busy
 * flag); then it ends. (Placed in levels $02, $08 and $18.)
 * HOW: ACTOR_09 bit 7 set (its kind's wait runs, Kind91FreeOff). The
 * escort's x pixels ($0937) and y pixels ($093A) = its own, its resume
 * address ($092E, a word) = $E826, its $47 ($0972) = its $07. JMP
 * through the four words after it by $07 * 2: 0, the escort's mode
 * ($0978) and busy flag ($0970) cleared; 1, 2, 3, the mode cleared;
 * each then JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML
 * of kind $92's stub (Kind92Enter, $83:937C) from the actor loop in the
 * start state.
 * Exit: never returns: ends in ActorEndPlaced.
 */
void abi_Kind92Start(void)
{
    lda8_v(0x80);
    tsb8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(ESCORT_X_PIXELS));
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_abs(ESCORT_Y_PIXELS));
    lda16_v(K92_ESCORT_RESUME);
    sta16(ea_abs(ESCORT_RESUME));
    op_sep(0x20);
    lda8(ea_dp(ACTOR_07));
    sta8(ea_abs(ESCORT_47));
    ACC8(asl);
    tax8();
    switch (cpu.x) {                       /* JMP (K92_WAYS,X) */
    case 0:
        stz8(ea_abs(ESCORT_MODE));
        stz8(ea_abs(ESCORT_BUSY));
        abi_ActorEndPlaced();
        return;
    case 2:
    case 4:
    case 6:
        stz8(ea_abs(ESCORT_MODE));
        abi_ActorEndPlaced();
        return;
    default:                               /* past the table's 4 words: no $07 names one */
        jump_routine(0xA50000u | rd16(0xA50000u | (uint16_t)(K92_WAYS + cpu.x)));
        return;
    }
}
