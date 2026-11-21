/*
 * pull_return.c: two helpers of the actors that ride Aladdin along, which keep their caller's return address (PLA, in GOTO_BACK) while
 * they wait and go back to it by ActorBackToCaller; nothing in the ROM calls either. no script runs either: a program of three
 * instructions poked into work RAM that an actor record's resume address starts. Written instruction for instruction from the
 * assembly (pull_return.asm).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorMoveY(void);
void abi_ActorCarryPlayer(void);
void abi_ActorBackToCaller(void);

#define GOTO_BACK 0x4Eu             /* the caller's return address, kept while it waits (word) */
#define CARRY_FRAMES ACTOR_40       /* frames left of a stretch */
#define CARRY_REST ACTOR_41         /* frames of the rest after the moving stretch; the bob's half length */
#define BOB_COUNT 0x47u             /* frames of the bob left */
#define BOB_STEP 0x45u              /* its y step a frame (word) */

/* A JSL at `site` to a routine whose C uses the stack: its return address on the stack while the callee runs. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:FBB5 ActorCarryMoveWait
 * @actor
 * WHAT IT DOES: A helper that nothing calls: for $40 frames the actor
 * moves by its vertical speed carrying Aladdin if it carries him, then
 * waits $41 frames, then goes back to its caller.
 * HOW: The return address pulled into GOTO_BACK (16-bit); each frame (BRK
 * first) ActorMoveY and ActorCarryPlayer, CARRY_FRAMES down to 0; then
 * CARRY_FRAMES = CARRY_REST and a BRK a frame until it is 0; on at
 * ActorBackToCaller.
 * Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit:
 * as ActorBackToCaller.
 */
void abi_ActorCarryMoveWait(void)
{
    op_rep(P_M);
    pla16();
    sta16(ea_dp(GOTO_BACK));
    op_sep(P_M);
    do {
        actor_yield(0x83FBBCu);
        jsl_call(0x83FBBEu, abi_ActorMoveY);
        call_frame(0xFBC4, abi_ActorCarryPlayer);
        MEM8(dec, ea_dp(CARRY_FRAMES));
    } while (!IF_Z);
    lda8(ea_dp(CARRY_REST));
    sta8(ea_dp(CARRY_FRAMES));
    do {
        actor_yield(0x83FBCDu);
        MEM8(dec, ea_dp(CARRY_FRAMES));
    } while (!IF_Z);
    abi_ActorBackToCaller();
}

/*
 * @routine 83:FC23 ActorCarryBob
 * @actor
 * WHAT IT DOES: A helper that nothing calls: the actor bobs in y by X
 * pixels a frame, A frames in all, turning the step round every A / 2
 * frames, carrying Aladdin if it carries him, then goes back to its
 * caller.
 * HOW: BOB_COUNT = A, CARRY_REST = A / 2; the return address pulled into
 * GOTO_BACK; BOB_STEP = X (16-bit). Each stretch CARRY_FRAMES = CARRY_REST;
 * each frame ActorCarryPlayer, BRK, BOB_COUNT down (at 0: on at
 * ActorBackToCaller), y pixels (16-bit) plus BOB_STEP, CARRY_FRAMES down;
 * at the end of a stretch BOB_STEP negated.
 * Entry: M 8-bit; A = the frames; X 8-bit = the step; D = the actor's
 * record; DB = $80; called by JSR. Exit: as ActorBackToCaller.
 */
void abi_ActorCarryBob(void)
{
    sta8(ea_dp(BOB_COUNT));
    ACC8(lsr);
    sta8(ea_dp(CARRY_REST));
    op_rep(P_M);
    pla16();
    sta16(ea_dp(GOTO_BACK));
    txa16();
    sta16(ea_dp(BOB_STEP));
    for (;;) {
        op_sep(P_M);
        lda8(ea_dp(CARRY_REST));
        sta8(ea_dp(CARRY_FRAMES));
        do {
            call_frame(0xFC38, abi_ActorCarryPlayer);
            actor_yield(0x83FC39u);
            MEM8(dec, ea_dp(BOB_COUNT));
            if (IF_Z) {
                abi_ActorBackToCaller();
                return;
            }
            op_rep(P_M);
            clc();
            lda16(ea_dp(ACTOR_Y + 1));
            adc16_v(rd16(ea_dp(BOB_STEP)));
            sta16(ea_dp(ACTOR_Y + 1));
            op_sep(P_M);
            MEM8(dec, ea_dp(CARRY_FRAMES));
        } while (!IF_Z);
        op_rep(P_M);
        lda16(ea_dp(BOB_STEP));
        eor16_v(0xFFFF);
        ACC16(inc);
        sta16(ea_dp(BOB_STEP));
    }
}
