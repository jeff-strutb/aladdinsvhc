/*
 * kind65.c: actor kind $65, the continues counter shown in play: its
 * picture is the count of continues left, changed as the count changes;
 * with none left as it starts, it ends. Kinds $61, $64 and $6C go on here
 * in their second state.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorEndPlaced(void);

#define K65_SHOWN 0x40u         /* the count it shows */
#define K65_FIRST_PICTURE 0x07u /* the animation for 0; the count is added */
#define CONTINUES 0x035Bu       /* continues left (3 at the start of a game) */

/*
 * @routine 83:E5BB Kind65Start
 * @actor
 * WHAT IT DOES: Actor kind $65 (and the second state of kinds $61, $64
 * and $6C): shows the continues left as its animation (7 plus the count),
 * changing it when the count changes; with none left as it starts, it
 * ends for good.
 * HOW: $08 bit 1, $09 bits 7 and 4 set; its look $0600; facing and flip
 * 0. Continues 0: JMP ActorEndPlaced. Else K65_SHOWN the count, AnimStart
 * with it plus 7; each frame (BRK first) until the count differs from
 * K65_SHOWN, then the count again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $65 in state $0C (the table at $83:8CAB),
 * and for kinds $61, $64, $6C in a state with bit 7 set ($83:8DFD).
 * Exit: never returns.
 */
void abi_Kind65Start(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(0x0600);
    sta16(ea_dp(ACTOR_LOOK));
    op_sep(0x20);
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    lda8(ea_abs(CONTINUES));
    if (IF_Z) {
        abi_ActorEndPlaced();
        return;
    }
    for (;;) {
        sta8(ea_dp(K65_SHOWN));
        clc();
        adc8_v(K65_FIRST_PICTURE);
        jsl_frame(0x83E5DDu);
        abi_AnimStart();
        cpu.s = (uint16_t)(cpu.s + 3);
        do {
            actor_yield(0x83E5E1u);
            lda8(ea_abs(CONTINUES));
            cmp8_v(rd8(ea_dp(K65_SHOWN)));
        } while (IF_Z);
    }
}
