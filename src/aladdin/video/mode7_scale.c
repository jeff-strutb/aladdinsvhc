/*
 * mode7_scale.c: a Mode 7 matrix element scaled, through the PPU's
 * multiplier, for the matrix the set-up at $80:85E2 writes.
 */
#include "machine.h"
#include "ops65.h"
void abi_Mode7MatrixWrite(void);
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

/*
 * @routine 80:85E2 Mode7MatrixWrite
 * WHAT IT DOES: Writes the Mode 7 matrix and centre to the PPU, each
 * element scaled by its own scale first when the NMI's mode is 1; nothing
 * when it is 0.
 * HOW: nmi_ppu_mode 0: returns. 1: Mode7ElementScale for D, C, B and A
 * with their scales ($0825-$0828), the results (low, then high byte) to
 * M7D, M7C, M7A and M7B. Else the matrix shadow ($038D) as it is, a byte
 * pair to each of M7A-M7D. Then the centre ($0395) to M7X and M7Y.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $2100-$213F are the PPU's and
 * $0000-$1FFF work RAM; called by JSR from the NMI. Exit: M 8-bit, X
 * 8-bit; written: A low byte = the centre's last byte.
 */
void abi_Mode7MatrixWrite(void)
{
    lda8(ea_abs(0x0829));  /* 8085E2 lda */
    if (IF_Z) goto L_80861C;  /* 8085E5 beq */
    ACC8(dec);  /* 8085E7 dec */
    if (IF_Z) goto L_80861D;  /* 8085E8 beq */
    lda8(ea_abs(0x038D));  /* 8085EA lda */
    sta8(ea_abs(M7A));  /* 8085ED sta */
    lda8(ea_abs(0x038E));  /* 8085F0 lda */
    sta8(ea_abs(M7A));  /* 8085F3 sta */
    lda8(ea_abs(0x038F));  /* 8085F6 lda */
    sta8(ea_abs(M7B));  /* 8085F9 sta */
    lda8(ea_abs(0x0390));  /* 8085FC lda */
    sta8(ea_abs(M7B));  /* 8085FF sta */
    lda8(ea_abs(0x0391));  /* 808602 lda */
    sta8(ea_abs(M7C));  /* 808605 sta */
    lda8(ea_abs(0x0392));  /* 808608 lda */
    sta8(ea_abs(M7C));  /* 80860B sta */
    lda8(ea_abs(0x0393));  /* 80860E lda */
    sta8(ea_abs(M7D));  /* 808611 sta */
    lda8(ea_abs(0x0394));  /* 808614 lda */
    sta8(ea_abs(M7D));  /* 808617 sta */
    goto L_80865F;  /* 80861A bra */
L_80861C:
    return;  /* 80861C rts */
L_80861D:
    op_rep(0x20);  /* 80861D rep */
    lda16(ea_abs(0x0393));  /* 80861F lda */
    ldx8(ea_abs(0x0828));  /* 808622 ldx */
    abi_Mode7ElementScale();  /* 808625 jsr */
    sty8(ea_abs(M7D));  /* 808628 sty */
    stx8(ea_abs(M7D));  /* 80862B stx */
    lda16(ea_abs(0x0391));  /* 80862E lda */
    ldx8(ea_abs(0x0827));  /* 808631 ldx */
    abi_Mode7ElementScale();  /* 808634 jsr */
    sty8(ea_abs(M7C));  /* 808637 sty */
    stx8(ea_abs(M7C));  /* 80863A stx */
    lda16(ea_abs(0x038F));  /* 80863D lda */
    ldx8(ea_abs(0x0826));  /* 808640 ldx */
    abi_Mode7ElementScale();  /* 808643 jsr */
    phy8();  /* 808646 phy */
    phx8();  /* 808647 phx */
    lda16(ea_abs(0x038D));  /* 808648 lda */
    ldx8(ea_abs(0x0825));  /* 80864B ldx */
    abi_Mode7ElementScale();  /* 80864E jsr */
    sty8(ea_abs(M7A));  /* 808651 sty */
    stx8(ea_abs(M7A));  /* 808654 stx */
    plx8();  /* 808657 plx */
    ply8();  /* 808658 ply */
    sty8(ea_abs(M7B));  /* 808659 sty */
    stx8(ea_abs(M7B));  /* 80865C stx */
L_80865F:
    op_sep(0x20);  /* 80865F sep */
    lda8(ea_abs(0x0395));  /* 808661 lda */
    sta8(ea_abs(M7X));  /* 808664 sta */
    lda8(ea_abs(0x0396));  /* 808667 lda */
    sta8(ea_abs(M7X));  /* 80866A sta */
    lda8(ea_abs(0x0397));  /* 80866D lda */
    sta8(ea_abs(M7Y));  /* 808670 sta */
    lda8(ea_abs(0x0398));  /* 808673 lda */
    sta8(ea_abs(M7Y));  /* 808676 sta */
    return;  /* 808679 rts */
}
