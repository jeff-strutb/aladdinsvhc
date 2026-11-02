/*
 * kind_5e.c: actor kind $5E: a brief show of its animation, then its end.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);

#define KIND5E_LEFT 0x40u  /* frames left */

/*
 * @routine 83:E42A Kind5EBrief
 * @actor
 * WHAT IT DOES: Actor kind $5E: shows animation $0C for $23 frames, then
 * ends (its $07 bit 2 picks the same either way).
 * HOW: Its look $0600; $08 bits 0-1 and $09 bits $90 set; animation $0C
 * (X) and $40 = $23 (Y), the same whether bit 2 of $07 is set; AnimStart;
 * $23 frames (BRK first, $40 down); then the jump at $83:E461 into
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $5E in state $0C. Exit: never returns;
 * ends in ActorEndPlaced.
 */
void abi_Kind5EBrief(void)
{
    op_rep(0x20);  /* 83E42A rep */
    lda16_v(0x0600);  /* 83E42C lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 83E42F sta */
    op_sep(0x20);  /* 83E431 sep */
    lda8_v(0x03);  /* 83E433 lda */
    tsb8(ea_dp(ACTOR_08));  /* 83E435 tsb */
    lda8_v(0x90);  /* 83E437 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83E439 tsb */
    ldx8_v(0x0C);  /* 83E43B ldx */
    ldy8_v(0x23);  /* 83E43D ldy */
    lda8(ea_dp(ACTOR_07));  /* 83E43F lda */
    and8_v(0x04);  /* 83E441 and */
    if (IF_Z) goto L_83E449;  /* 83E443 beq */
    ldx8_v(0x0C);  /* 83E445 ldx */
    ldy8_v(0x23);  /* 83E447 ldy */
L_83E449:
    txa8();  /* 83E449 txa */
    sty8(ea_dp(KIND5E_LEFT));  /* 83E44A sty */
    abi_AnimStart();  /* 83E44C jsl */
L_83E450:
    actor_yield(0x83E450u);  /* 83E450 brk */
    MEM8(dec, ea_dp(KIND5E_LEFT));  /* 83E452 dec */
    if (!IF_Z) goto L_83E450;  /* 83E454 bne */
    goto L_83E461;  /* 83E456 bra */
L_83E461:
    abi_ActorEndPlaced(); return;  /* 83E461 jml */
}
