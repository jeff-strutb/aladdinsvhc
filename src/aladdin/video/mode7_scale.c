/*
 * mode7_scale.c: a Mode 7 matrix element scaled, through the PPU's
 * multiplier, for the matrix the set-up at $80:85E2 writes.
 */
#include "machine.h"
#include "ops65.h"
#include "hw.h"

/*
 * @routine 80:867A Mode7ElementScale
 * WHAT IT DOES: Scales a Mode 7 matrix element: A times X / 256, for the
 * caller to write to a matrix register (low byte Y, then high byte X).
 * HOW: The PPU's signed multiplier: the element negated goes to M7A (low
 * byte, then high), X negated to M7B; the product's middle and high bytes
 * are read back into Y and X (both factors negated, so X = $80 counts as
 * +128). The high byte is written to M7B once more.
 * Entry: M 16-bit, X 8-bit; A = the element; X = the scale; DB a bank
 * whose $2100-$213F are the PPU's. Exit: A = X negated (16 bits); Y = the
 * product's middle byte, X = its high byte; N and Z from X; M 16-bit, X
 * 8-bit; C, V and the other flags unchanged.
 */
void abi_Mode7ElementScale(void)
{
    eor16_v(0xFFFF);
    ACC16(inc);
    tay8();
    sty8(ea_abs(M7A));
    xba();
    tay8();
    sty8(ea_abs(M7A));
    txa16();
    eor16_v(0xFFFF);
    ACC16(inc);
    tax8();
    stx8(ea_abs(M7B));
    ldy8(ea_abs(MPYM));
    ldx8(ea_abs(MPYH));
    stx8(ea_abs(M7B));
}
