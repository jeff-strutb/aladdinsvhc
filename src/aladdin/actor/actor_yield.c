/*
 * actor_yield.c: where an actor's BRK or COP leads (the vectors' code,
 * system/interrupts.c): the actor waits, its resume place kept in its
 * record, and the actor loop goes on (docs/aladdin/ACTORS.md).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorWaitEntry(void);

#define AY_PLAYER_LOOP_ON 0x08DAu   /* 1 while the player's record is run */
#define AY_WAIT 0x01u               /* frames left to wait (the record's $01) */
#define AY_RESUME_P 0x02u           /* where it resumes: P, then PC and bank */
#define ACTOR_LOOP_BACK 0x8394BFu   /* ActorLoopBack */
#define PLAYER_LOOP_BACK 0x83948Bu  /* PlayerLoopBack */

/*
 * @routine 80:88E5 ActorYieldEntry
 * @interrupt
 * WHAT IT DOES: An actor's BRK: it waits one frame (ActorWaitEntry with A
 * = 1).
 * HOW: 8-bit registers, A = 1, on into ActorWaitEntry.
 * Entry: a BRK's frame on the stack (P, then the PC past the BRK's
 * signature byte and its bank); D = the actor's record; reached by the
 * BRK vector's JML. Exit: as ActorWaitEntry.
 */
void abi_ActorYieldEntry(void)
{
    op_sep(0x30);
    lda8_v(0x01);
    abi_ActorWaitEntry();
}

/*
 * @routine 80:88E9 ActorWaitEntry
 * @interrupt
 * WHAT IT DOES: An actor waits A frames: its record marked waiting with
 * the count, the place it resumes at kept in it, and the actor loop goes
 * on with the next record (or ends the player's turn).
 * HOW: CLI; AY_WAIT = A; state 1 (waiting); the frame pulled (16-bit): P
 * and PC low into AY_RESUME_P, PC high and bank into AY_RESUME_P + 2;
 * 8-bit registers; AY_PLAYER_LOOP_ON zero: JML ActorLoopBack, else JML
 * PlayerLoopBack.
 * Entry: a COP's frame on the stack (or a BRK's, from ActorYieldEntry); D
 * = the actor's record; M 8-bit with A low byte = the frames; reached by
 * the COP vector's JML. Exit: never returns: in the actor loop with S
 * above the frame, M and X 8-bit, A = AY_PLAYER_LOOP_ON.
 */
void abi_ActorWaitEntry(void)
{
    cli();
    sta8(ea_dp(AY_WAIT));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_STATE));
    op_rep(0x20);
    pla16();
    sta16(ea_dp(AY_RESUME_P));
    pla16();
    sta16(ea_dp(AY_RESUME_P + 2));
    op_sep(0x30);
    lda8(ea_abs(AY_PLAYER_LOOP_ON));
    actor_leave(IF_Z ? ACTOR_LOOP_BACK : PLAYER_LOOP_BACK);
}
