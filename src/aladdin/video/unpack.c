/*
 * unpack.c: setting up the unpacking of packed data (graphics) into bank
 * $7F.
 *
 * An unpack reads its source through the long pointer at $65 (offset 0
 * in the source's bank, the offset itself in Y) and writes 8 bytes a
 * group to bank $7F at X; $69 counts the groups.
 */
#include "machine.h"
#include "ops65.h"
void abi_UnpackSetBegin(void);
void abi_UnpackBegin(void);
#include "wram.h"

#define UNPACK_SOURCE 0x65u        /* direct page (D = 0): the source's pointer (long) */
#define UNPACK_GROUPS 0x69u        /* direct page: the 8-byte groups left (word) */
#define UNPACK_SETS 0xF039u        /* bank $80: per set, 7 bytes: destination, source offset (words), bank, length (word) */

/*
 * @routine 81:BB3D UnpackBegin
 * WHAT IT DOES: Starts an unpack into bank $7F: the source offset in Y
 * and the data bank at $7F, and says whether there is anything to unpack.
 * HOW: Y = A; DB = $7F; A = the group count's two bytes ORed.
 * Entry: M 16-bit, X 16-bit; A = the source offset; D = 0; $65-$67 = the
 * source's bank pointer; $69 = the 8-byte groups to unpack (word); called
 * by JSR. Exit: M 8-bit; Y = the source offset; DB = $7F; Z set when
 * there are no groups, N from the OR; X, C, V unchanged.
 */
void abi_UnpackBegin(void)
{
    tay16();  /* 81BB3D tay */
    op_sep(0x20);  /* 81BB3E sep */
    lda8_v(0x7F);  /* 81BB40 lda */
    pha8();  /* 81BB42 pha */
    plb();  /* 81BB43 plb */
    lda8(ea_dp(UNPACK_GROUPS));  /* 81BB44 lda */
    ora8_v(rd8(ea_dp(UNPACK_GROUPS + 1)));  /* 81BB46 ora */
    return;  /* 81BB48 rts */
}

/*
 * @routine 81:BB04 UnpackSetBegin
 * WHAT IT DOES: Sets up the unpacking of packed set A (the graphics sets
 * $81:BAF1 unpacks) into bank $7F.
 * HOW: Entry A of the 7-byte table at $80:F039 (A * 7, counting in scratch
 * $0C, with DB = $80 and D = 0): X = where it goes in bank $7F, the source
 * pointer at $65 = its bank (+4) with offset 0, $69 = its length (+5)
 * rounded up to 8-byte groups; then the source offset (+2) goes on into
 * UnpackBegin.
 * Entry: M 16-bit, X 16-bit; A low byte = the set; called by JSR. Exit:
 * as UnpackBegin; X = where the set goes; D = 0.
 */
void abi_UnpackSetBegin(void)
{
    and16_v(0x00FF);  /* 81BB04 and */
    sta16(ea_abs(SCRATCH));  /* 81BB07 sta */
    op_sep(0x20);  /* 81BB0A sep */
    lda8_v(0x80);  /* 81BB0C lda */
    pha8();  /* 81BB0E pha */
    plb();  /* 81BB0F plb */
    op_rep(0x30);  /* 81BB10 rep */
    lda16(ea_abs(SCRATCH));  /* 81BB12 lda */
    ACC16(asl);  /* 81BB15 asl */
    ACC16(asl);  /* 81BB16 asl */
    ACC16(asl);  /* 81BB17 asl */
    sec();  /* 81BB18 sec */
    sbc16_v(rd16(ea_abs(SCRATCH)));  /* 81BB19 sbc */
    tay16();  /* 81BB1C tay */
    lda16_v(0x0000);  /* 81BB1D lda */
    tcd();  /* 81BB20 tcd */
    ldx16(ea_absy(UNPACK_SETS));  /* 81BB21 ldx */
    stz16(ea_dp(UNPACK_SOURCE));  /* 81BB24 stz */
    lda16(ea_absy(UNPACK_SETS + 4));  /* 81BB26 lda */
    sta16(ea_dp(UNPACK_SOURCE + 2));  /* 81BB29 sta */
    lda16(ea_absy(UNPACK_SETS + 5));  /* 81BB2B lda */
    clc();  /* 81BB2E clc */
    adc16_v(0x0007);  /* 81BB2F adc */
    and16_v(0xFFF8);  /* 81BB32 and */
    ACC16(lsr);  /* 81BB35 lsr */
    ACC16(lsr);  /* 81BB36 lsr */
    ACC16(lsr);  /* 81BB37 lsr */
    sta16(ea_dp(UNPACK_GROUPS));  /* 81BB38 sta */
    lda16(ea_absy(UNPACK_SETS + 2));  /* 81BB3A lda */
    abi_UnpackBegin();
}
