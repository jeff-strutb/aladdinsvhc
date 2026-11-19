/*
 * multiply.c: the CPU's 8x8 multiplier and 16/8 divider.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "hw.h"
#include "ops65.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:A7E6 MultiplyBy
 * WHAT IT DOES: Multiplies the factor already in WRMPYA by A.
 * HOW: Writes A to WRMPYB, waits the multiplier's time and reads the
 * 16-bit product.
 * Entry: M 8-bit, X 16-bit; A low byte = the second factor; DB a bank
 * whose $4200-$421F are the CPU's; called by JSR. Exit: Y = the product;
 * N, Z from it; A, X, C, V unchanged.
 */
void abi_MultiplyBy(void)
{
    wr8(abs_db(WRMPYB & 0xFFFF), (uint8_t)cpu.a);
    cpu.y = rd16(abs_db(RDMPYL & 0xFFFF));
    cpu.p = nz16(cpu.p, cpu.y);
}

/* Width-dependent stores and loads of A, X and Y (STA, STX, STY, LDA,
   LDY at the widths M and X give). */
static void sta_w(uint16_t addr) { if (cpu.p & P_M) wr8(abs_db(addr), (uint8_t)cpu.a); else wr16(abs_db(addr), cpu.a); }
static void stx_w(uint16_t addr) { if (cpu.p & P_X) wr8(abs_db(addr), (uint8_t)cpu.x); else wr16(abs_db(addr), cpu.x); }
static void sty_w(uint16_t addr) { if (cpu.p & P_X) wr8(abs_db(addr), (uint8_t)cpu.y); else wr16(abs_db(addr), cpu.y); }
static void lda_w(uint16_t addr)
{
    if (cpu.p & P_M) {
        uint8_t v = rd8(abs_db(addr));

        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
    } else {
        cpu.a = rd16(abs_db(addr));
        cpu.p = nz16(cpu.p, cpu.a);
    }
}
static void ldy_w(uint16_t addr)
{
    if (cpu.p & P_X) {
        cpu.y = rd8(abs_db(addr));
        cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
    } else {
        cpu.y = rd16(abs_db(addr));
        cpu.p = nz16(cpu.p, cpu.y);
    }
}

/* Helpers no code refers to: each runs only in the unit calls of
   cov_helpers_unused.box. */

/*
 * @routine 81:A7F1 DivideBy
 * WHAT IT DOES: Divides A by Y with the CPU's divider: the quotient in A,
 * the remainder in Y. No code refers to it.
 * HOW: A to WRDIVL and Y to WRDIVB (at their widths: with X 16-bit the
 * high byte goes to $4207), eight NOPs while it works, then A from RDDIVL
 * and Y from RDMPYL.
 * Entry: M 16-bit and X 8-bit for a 16 by 8 division (either width works
 * as written); DB a bank whose $4200-$421F are the CPU's; called by JSL.
 * Exit: A = the quotient, Y = the remainder (at their widths); N, Z from
 * Y; X, C, V unchanged.
 */
void abi_DivideBy(void)
{
    sta_w(WRDIVL & 0xFFFF);
    sty_w(WRDIVB & 0xFFFF);
    lda_w(RDDIVL & 0xFFFF);
    ldy_w(RDMPYL & 0xFFFF);
}

/*
 * @routine 81:A806 MultiplyXA
 * WHAT IT DOES: Multiplies X's low byte by A: the 16-bit product in A. No
 * code refers to it.
 * HOW: X to WRMPYA (with X 16-bit its high byte to WRMPYB too, which
 * MultiplyBy then writes over), MultiplyBy, then RDMPYH, XBA, and the
 * product's low byte from Y.
 * Entry: M 8-bit, X 16-bit (as MultiplyBy); A low byte = the second
 * factor; DB a bank whose $4200-$421F are the CPU's; called by JSL. Exit:
 * A = the product (16 bits); Y = the product; N, Z from its low byte; X,
 * C, V unchanged.
 */
void abi_MultiplyXA(void)
{
    uint8_t hi;

    stx_w(WRMPYA & 0xFFFF);
    call_frame(0xA80B, abi_MultiplyBy);
    hi = rd8(abs_db(RDMPYH & 0xFFFF));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | hi);
    cpu.p = nz8(cpu.p, hi);
    op_xba();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)cpu.y);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.y);
}

/*
 * @routine 81:A812 MultiplyWordByte
 * WHAT IT DOES: Multiplies the word at $0C by the byte at $0E (scratch, in
 * the data bank) into three bytes at $0C-$0E. No code refers to it.
 * HOW: With X the word at $0E in WRMPYA (and WRMPYB): $0C times it (high
 * byte to $0F, the product to $10-$11), then $0D times it (high byte to
 * $11, the product to $12-$13); then from $0E down with the carry: $0E =
 * $11, $0D = $12 + $0F, $0C = $10.
 * Entry: M 8-bit, X 16-bit (as MultiplyBy); DB a bank whose $0000-$1FFF is
 * work RAM and $4200-$421F the CPU's; called by JSL. Exit: A low byte =
 * the byte at $0C; N, V, Z, C from its ADC; X = the word at $0E as it was;
 * Y = the second product; scratch $0F-$13 as above.
 */
void abi_MultiplyWordByte(void)
{
    uint8_t v, w;

    v = rd8(abs_db(0x000C));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = rd16(abs_db(0x000E));
    wr16(abs_db(WRMPYA & 0xFFFF), cpu.x);
    abi_MultiplyBy();
    v = rd8(abs_db(RDMPYH & 0xFFFF));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(abs_db(0x000F), v);
    wr16(abs_db(0x0010), cpu.y);
    v = rd8(abs_db(0x000D));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr16(abs_db(WRMPYA & 0xFFFF), cpu.x);
    abi_MultiplyBy();
    v = rd8(abs_db(RDMPYH & 0xFFFF));
    wr8(abs_db(0x0011), v);
    wr16(abs_db(0x0012), cpu.y);
    cpu.p &= (uint8_t)~P_C;
    v = rd8(abs_db(0x0011));
    v = op_adc8(v, 0x00, &cpu.p);
    wr8(abs_db(0x000E), v);
    v = rd8(abs_db(0x0012));
    w = rd8(abs_db(0x000F));
    v = op_adc8(v, w, &cpu.p);
    wr8(abs_db(0x000D), v);
    v = rd8(abs_db(0x0010));
    v = op_adc8(v, 0x00, &cpu.p);
    wr8(abs_db(0x000C), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}
