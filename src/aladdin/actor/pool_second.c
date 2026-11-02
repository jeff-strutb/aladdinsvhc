/*
 * pool_second.c: the second pool of records ($09CB-$0B0B): all of it freed at once.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorFree(void);
void abi_ActorTileSlotRelease(void);

/*
 * @routine 83:95C1 SecondPoolFreeAll
 * WHAT IT DOES: Frees every record of the second pool that is in use (as a
 * level ends).
 * HOW: D steps through the five records from $09CB: one whose state is
 * nonzero gets ActorFree and ActorTileSlotRelease. D and P kept.
 * Entry: any widths; DB a bank whose $0000-$1FFF is work RAM; called by
 * JSL (from 81:E7B5). Exit: D and P as at entry; A, X, Y as the last
 * record left them.
 */
void abi_SecondPoolFreeAll(void)
{
    phd();  /* 8395C1 phd */
    php();  /* 8395C2 php */
    op_rep(0x20);  /* 8395C3 rep */
    lda16_v(0x09CB);  /* 8395C5 lda */
    tcd();  /* 8395C8 tcd */
L_8395C9:
    ldx8(ea_dp(0x00));  /* 8395C9 ldx */
    if (IF_Z) goto L_8395D7;  /* 8395CB beq */
    op_sep(0x20);  /* 8395CD sep */
    abi_ActorFree();  /* 8395CF jsr */
    abi_ActorTileSlotRelease();  /* 8395D2 jsr */
    op_rep(0x20);  /* 8395D5 rep */
L_8395D7:
    clc();  /* 8395D7 clc */
    tdc();  /* 8395D8 tdc */
    adc16_v(0x0050);  /* 8395D9 adc */
    tcd();  /* 8395DC tcd */
    cmp16_v(0x0B5B);  /* 8395DD cmp */
    if (!IF_Z) goto L_8395C9;  /* 8395E0 bne */
    plp();  /* 8395E2 plp */
    pld();  /* 8395E3 pld */
    return;  /* 8395E4 rtl */
}

