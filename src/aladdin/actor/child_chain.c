/*
 * child_chain.c: an actor's chain of children: each new child linked to
 * the one before, placed on its parent, numbered. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateChild(void);

#define CHAIN_LAST 0x40u  /* word: the record of the last child created; in a child, its number */
#define CHAIN_COUNT 0x42u  /* children created so far */
#define CHAIN_BEFORE 0x4Eu  /* in a child: the record of the child after it */

/*
 * @routine 84:94EE ChildChainAdd
 * WHAT IT DOES: Creates a child of kind A and adds it to the actor's
 * chain: the new child's $37 is the last one's record, the last one's $4E
 * the new one's; the child is put at the actor's position and numbered
 * (its $40 = the count so far); it becomes the last.
 * HOW: ActorCreateChild (X = the new record; not tested); $40 nonzero: new
 * $37 = $40, $40's $4E = the new record (16-bit). The new x and y pixels =
 * the actor's; its $40 = $42; $42 up; $40 = the new record.
 * Entry: M 8-bit, X 8-bit; A = the kind; D = the parent's record; DB =
 * $84; called by JSR (84:94AE and the eight calls after it). Exit: M
 * 8-bit, X 8-bit; X = the new record's low byte.
 */
void abi_ChildChainAdd(void)
{
    abi_ActorCreateChild();  /* 8494EE jsl */
    ldy16(ea_dp(CHAIN_LAST));  /* 8494F2 ldy */
    if (IF_Z) goto L_849502;  /* 8494F4 beq */
    op_rep(0x20);  /* 8494F6 rep */
    tya16();  /* 8494F8 tya */
    sta16(ea_absx(ACTOR_NEXT));  /* 8494F9 sta */
    txa16();  /* 8494FC txa */
    sta16(ea_absy(CHAIN_BEFORE));  /* 8494FD sta */
    op_sep(0x20);  /* 849500 sep */
L_849502:
    op_rep(0x20);  /* 849502 rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 849504 lda */
    sta16(ea_absx(ACTOR_X + 1));  /* 849506 sta */
    lda16(ea_dp(ACTOR_Y + 1));  /* 849509 lda */
    sta16(ea_absx(ACTOR_Y + 1));  /* 84950B sta */
    op_sep(0x20);  /* 84950E sep */
    lda8(ea_dp(CHAIN_COUNT));  /* 849510 lda */
    sta8(ea_absx(CHAIN_LAST));  /* 849512 sta */
    MEM8(inc, ea_dp(CHAIN_COUNT));  /* 849515 inc */
    stx16(ea_dp(CHAIN_LAST));  /* 849517 stx */
    op_sep(0x10);  /* 849519 sep */
    return;  /* 84951B rts */
}
