/*
 * in_view.c: which actors are near enough the view to be hit, and the
 * lists of them, by their low two bits of $08, that other code walks.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorsInView(void);
#include "actor.h"

/*
 * @routine 81:9198 ActorsInView
 * WHAT IT DOES: Marks which actors can be hit this frame (bit 6 of $09):
 * those whose box, picked by their $12, holds the view's point; and lists
 * those with bit 3 of $08 set, by their $08's low two bits.
 * HOW: The counts cleared and the lists' next offsets set from their
 * starts; then for each of the 35 records from actor_escort that is in use:
 * the view point less its x and y, plus the box's offsets, under the box's
 * width and height sets the bit, else clears it; with the bit set and bit
 * 3 of $08 set, the record's address goes on its list and the list's count
 * steps up.
 * Entry: DB = $81; called by JSL. Exit: P and D as at entry; A, X, Y as
 * the walk left them.
 */
void abi_ActorsInView(void)
{
    php();  /* 819198 php */
    phd();  /* 819199 phd */
    op_rep(0x30);  /* 81919A rep */
    stz16(ea_abs(0x1592));  /* 81919C stz */
    stz16(ea_abs(0x1594));  /* 81919F stz */
    stz16(ea_abs(0x1596));  /* 8191A2 stz */
    stz16(ea_abs(0x1598));  /* 8191A5 stz */
    lda16(ea_abs(0xBA2A));  /* 8191A8 lda */
    sta16(ea_abs(0x159A));  /* 8191AB sta */
    lda16(ea_abs(0xBA2C));  /* 8191AE lda */
    sta16(ea_abs(0x159C));  /* 8191B1 sta */
    lda16(ea_abs(0xBA2E));  /* 8191B4 lda */
    sta16(ea_abs(0x159E));  /* 8191B7 sta */
    lda16(ea_abs(0xBA30));  /* 8191BA lda */
    sta16(ea_abs(0x15A0));  /* 8191BD sta */
    ldy16_v(0x0023);  /* 8191C0 ldy */
    lda16_v(0x092B);  /* 8191C3 lda */
    tcd();  /* 8191C6 tcd */
L_8191C7:
    lda16(ea_dp(0x00));  /* 8191C7 lda */
    and16_v(0x00FF);  /* 8191C9 and */
    if (IF_Z) goto L_819224;  /* 8191CC beq */
    lda16(ea_dp(0x12));  /* 8191CE lda */
    and16_v(0x00FF);  /* 8191D0 and */
    tax16();  /* 8191D3 tax */
    sec();  /* 8191D4 sec */
    lda16(ea_abs(0x1A44));  /* 8191D5 lda */
    sbc16_v(rd16(ea_dp(0x0C)));  /* 8191D8 sbc */
    clc();  /* 8191DA clc */
    adc16_v(rd16(ea_absx(0xBA02)));  /* 8191DB adc */
    cmp16_v(rd16(ea_absx(0xBA04)));  /* 8191DE cmp */
    if (IF_C) goto L_81921D;  /* 8191E1 bcs */
    sec();  /* 8191E3 sec */
    lda16(ea_abs(0x1A46));  /* 8191E4 lda */
    sbc16_v(rd16(ea_dp(0x0F)));  /* 8191E7 sbc */
    clc();  /* 8191E9 clc */
    adc16_v(rd16(ea_absx(0xBA06)));  /* 8191EA adc */
    cmp16_v(rd16(ea_absx(0xBA08)));  /* 8191ED cmp */
    if (IF_C) goto L_81921D;  /* 8191F0 bcs */
    lda16(ea_dp(0x08));  /* 8191F2 lda */
    ora16_v(0x4000);  /* 8191F4 ora */
    sta16(ea_dp(0x08));  /* 8191F7 sta */
    lda16(ea_dp(0x08));  /* 8191F9 lda */
    bit16_i(0x0008);  /* 8191FB bit */
    if (IF_Z) goto L_819224;  /* 8191FE beq */
    phy16();  /* 819200 phy */
    and16_v(0x0003);  /* 819201 and */
    ACC16(asl);  /* 819204 asl */
    tax16();  /* 819205 tax */
    ldy16(ea_absx(0x159A));  /* 819206 ldy */
    tdc();  /* 819209 tdc */
    sta16(ea_absy(0x1492));  /* 81920A sta */
    MEM16(inc, ea_absx(0x159A));  /* 81920D inc */
    MEM16(inc, ea_absx(0x159A));  /* 819210 inc */
    txy16();  /* 819213 txy */
    ldx16(ea_absy(0xBA32));  /* 819214 ldx */
    MEM16(inc, ea_absx(0x1592));  /* 819217 inc */
    ply16();  /* 81921A ply */
    goto L_819224;  /* 81921B bra */
L_81921D:
    lda16(ea_dp(0x08));  /* 81921D lda */
    and16_v(0xBFFF);  /* 81921F and */
    sta16(ea_dp(0x08));  /* 819222 sta */
L_819224:
    clc();  /* 819224 clc */
    tdc();  /* 819225 tdc */
    adc16_v(0x0050);  /* 819226 adc */
    tcd();  /* 819229 tcd */
    dey16();  /* 81922A dey */
    if (!IF_Z) goto L_8191C7;  /* 81922B bne */
    pld();  /* 81922D pld */
    plp();  /* 81922E plp */
    return;  /* 81922F rtl */
}
