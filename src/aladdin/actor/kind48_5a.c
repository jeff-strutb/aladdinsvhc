/*
 * kind48_5a.c: two still actors. Kind $48 (placed in no level, spawned
 * by no code) shows pose 0 for good; in its second state it ends. Kind $5A
 * (the Genie's screen, level $2F) shows the pose the scene asks for in
 * $00B5. Their waits are Kind48Wait and Kind5AWait (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_ActorEndPlaced(void);

#define K5A_29 0x29u            /* set to K5A_29_START at its start; purpose not yet established (word) */
#define K5A_29_START 0x8915u
#define K5A_POSE_CUE 0x00B5u    /* the Genie's screen: 0 pose 0, 1 pose 1, more pose 2 */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:CE14 Kind48Start
 * @actor
 * WHAT IT DOES: Actor kind $48: pose 0, still, for good (its wait
 * animates it). Placed in no level and spawned by no code: a record
 * restarted as the kind runs it (cov_kind48_5a).
 * HOW: $09 = $80; AnimStart 0; a BRK each frame.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $48 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind48Start(void)
{
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    jsl_call(0x83CE1Au, abi_AnimStart);
    for (;;)
        actor_yield(0x83CE1Eu);
}

/*
 * @routine 83:CE22 Kind48Second
 * @actor
 * WHAT IT DOES: Kind $48 in a state with bit 7 set (its second): it ends
 * for good. It cannot be hit ($09 bit 6 clear), so no play gives it that
 * state: a record put in state $8C runs it (cov_kind48_5a).
 * HOW: JMP ActorEndPlaced.
 * Entry: D = the actor's record; reached by the actor loop's jump for
 * kind $48 in a state with bit 7 set (the table at $83:8DFD).
 * Exit: never returns.
 */
void abi_Kind48Second(void)
{
    abi_ActorEndPlaced();
}

/*
 * @routine 83:DEBE Kind5AStart
 * @actor
 * WHAT IT DOES: Actor kind $5A of the Genie's screen: each frame the pose
 * its scene asks for in $00B5 (0: pose 0, 1: pose 1, 2 up: pose 2).
 * HOW: $08 bit 1, $09 bits 7, 5 and 4 set; redraw; $29 $8915; AnimStart
 * 0. Each turn: $00B5 0: BRK; 1: AnimStart 1, BRK; else AnimStart 2, BRK.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $5A in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind5AStart(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0xB0);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0xFF);
    sta8(ea_dp(ACTOR_REDRAW));
    op_rep(0x20);
    lda16_v(K5A_29_START);
    sta16(ea_dp(K5A_29));
    op_sep(0x20);
    lda8_v(0x00);
    jsl_call(0x83DED5u, abi_AnimStart);
    for (;;) {
        lda8(ea_abs(K5A_POSE_CUE));
        if (!IF_Z) {
            ACC8(dec);
            if (IF_Z) {
                lda8_v(0x01);
                jsl_call(0x83DEE3u, abi_AnimStart);
            } else {
                lda8_v(0x02);
                jsl_call(0x83DEEDu, abi_AnimStart);
            }
        }
        actor_yield(0x83DEE7u);
    }
}
