/*
 * text_number.c: a two-digit number written into the text layer's tilemap.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "hw.h"



/*
 * @routine 83:8BDE TextNumberTwoDigits
 * WHAT IT DOES: Writes the number X (0-99) as two digits into the text
 * layer's tilemap at the offset A (high byte) and Y (low byte), with
 * attribute $28, and has the NMI upload the tilemap.
 * HOW: The offset made in A (XBA, TYA); X divided by 10 by the CPU's
 * divider (eight NOPs while it works); tens (the quotient) + $30 at the
 * offset, ones (the remainder) + $30 two bytes on, $28 in both attribute
 * bytes; TEXT_TILEMAP_UPLOAD up.
 * Entry: M 8-bit, X 8-bit; X = the number; A = the offset's high byte; Y =
 * its low byte; DB a bank whose $4200-$421F are the CPU's; called by JSR
 * (83:8BDA, and by a jump from 83:8BA5). Exit: M 8-bit, X 8-bit; A = $28;
 * X = the offset's low byte; C clear.
 */
void abi_TextNumberTwoDigits(void)
{
    xba();  /* 838BDE xba */
    tya8();  /* 838BDF tya */
    stx8(ea_abs(WRDIVL));  /* 838BE0 stx */
    ldx8_v(0x0A);  /* 838BE3 ldx */
    stx8(ea_abs(WRDIVB));  /* 838BE5 stx */
    ;  /* 838BE8 nop */
    ;  /* 838BE9 nop */
    ;  /* 838BEA nop */
    ;  /* 838BEB nop */
    ;  /* 838BEC nop */
    ;  /* 838BED nop */
    ;  /* 838BEE nop */
    ;  /* 838BEF nop */
    op_rep(0x11);  /* 838BF0 rep */
    tax16();  /* 838BF2 tax */
    lda8(ea_abs(RDDIVL));  /* 838BF3 lda */
    adc8_v(0x30);  /* 838BF6 adc */
    sta8(ea_longx(TEXT_TILEMAP));  /* 838BF8 sta */
    lda8(ea_abs(RDMPYL));  /* 838BFC lda */
    clc();  /* 838BFF clc */
    adc8_v(0x30);  /* 838C00 adc */
    sta8(ea_longx(TEXT_TILEMAP + 2));  /* 838C02 sta */
    lda8_v(0x28);  /* 838C06 lda */
    sta8(ea_longx(TEXT_TILEMAP + 1));  /* 838C08 sta */
    sta8(ea_longx(TEXT_TILEMAP + 3));  /* 838C0C sta */
    op_sep(0x10);  /* 838C10 sep */
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));  /* 838C12 inc */
    return;  /* 838C15 rts */
}
