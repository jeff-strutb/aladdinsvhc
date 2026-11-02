/*
 * text_number.c: a two-digit number written into the text layer's tilemap.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint8_t number = (uint8_t)cpu.x, digit;

    op_xba();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)cpu.y);   /* A = the offset */
    nz8_of((uint8_t)cpu.y);
    wr8(dba(WRDIVL & 0xFFFF), number);    /* the number by 10 */
    x8_load(0x0A);
    wr8(dba(WRDIVB & 0xFFFF), 0x0A);
    op_rep(0x11);
    cpu.x = cpu.a;
    digit = op_adc8(rd8(dba(RDDIVL & 0xFFFF)), 0x30, &cpu.p);      /* tens */
    wr8((TEXT_TILEMAP + cpu.x) & 0xFFFFFF, digit);
    flag_set(P_C, 0);
    digit = op_adc8(rd8(dba(RDMPYL & 0xFFFF)), 0x30, &cpu.p);      /* units */
    wr8((TEXT_TILEMAP + 2 + cpu.x) & 0xFFFFFF, digit);
    a8_load(0x28);
    wr8((TEXT_TILEMAP + 1 + cpu.x) & 0xFFFFFF, 0x28);
    wr8((TEXT_TILEMAP + 3 + cpu.x) & 0xFFFFFF, 0x28);
    op_sep(0x10);
    digit = (uint8_t)(rd8(dba(TEXT_TILEMAP_UPLOAD)) + 1);
    wr8(dba(TEXT_TILEMAP_UPLOAD), digit);
    nz8_of(digit);
}
