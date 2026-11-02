/*
 * unpack.c: setting up the unpacking of packed data (graphics) into bank
 * $7F.
 *
 * An unpack reads its source through the long pointer at $65 (offset 0
 * in the source's bank, the offset itself in Y) and writes 8 bytes a
 * group to bank $7F at X; $69 counts the groups. A group is a flag byte
 * and a fill byte, then a byte for each set flag bit: bit 7 first, a set
 * bit takes the next literal byte, a clear one the fill byte. The source
 * runs on across banks: past $FFFF at $8000 of the next.
 */
#include "machine.h"
#include "ops65.h"
void abi_UnpackSetBegin(void);
void abi_UnpackBegin(void);
void abi_UnpackGroup(void);
#include "wram.h"

#define UNPACK_SOURCE 0x65u        /* direct page (D = 0): the source's pointer (long) */
#define UNPACK_FLAGS 0x0Cu         /* direct page: the group's flag byte, shifted out bit by bit */
#define UNPACK_FILL 0x0Du          /* direct page: the group's fill byte */
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

/*
 * @routine 81:BB49 UnpackGroup
 * WHAT IT DOES: Unpacks one 8-byte group of a packed asset to $7F:X and
 * counts it off.
 * HOW: Takes the flag and fill bytes from the source, then for each of the
 * 8 bytes shifts the flags: a set bit stores the next source byte, a clear
 * one the fill byte. Each source step past $FFFF goes on at $8000 of the
 * next bank. X moves on 8; the groups left go down by one.
 * Entry: M 8-bit, X 16-bit; D = 0; DB = $7F; X = the destination; Y = the
 * source offset in the bank at UNPACK_SOURCE + 2; called by JSR
 * (UnpackSet). Exit: X = the next destination, A = X (16 bits, from
 * TXA/ADC), Y the next source offset; N and Z from the groups-left
 * decrement (Z set: none left); C from the ADC; M 8-bit, X 16-bit.
 */
void abi_UnpackGroup(void)
{
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BB49 lda */
    sta8(ea_dp(UNPACK_FLAGS));  /* 81BB4B sta */
    iny16();  /* 81BB4D iny */
    if (!IF_Z) goto L_81BB55;  /* 81BB4E bne */
    ldy16_v(0x8000);  /* 81BB50 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BB53 inc */
L_81BB55:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BB55 lda */
    sta8(ea_dp(UNPACK_FILL));  /* 81BB57 sta */
    iny16();  /* 81BB59 iny */
    if (!IF_Z) goto L_81BB61;  /* 81BB5A bne */
    ldy16_v(0x8000);  /* 81BB5C ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BB5F inc */
L_81BB61:
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BB61 asl */
    if (IF_C) goto L_81BB69;  /* 81BB63 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BB65 lda */
    goto L_81BB73;  /* 81BB67 bra */
L_81BB69:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BB69 lda */
    iny16();  /* 81BB6B iny */
    if (!IF_Z) goto L_81BB73;  /* 81BB6C bne */
    ldy16_v(0x8000);  /* 81BB6E ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BB71 inc */
L_81BB73:
    sta8(ea_absx(0x0000));  /* 81BB73 sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BB76 asl */
    if (IF_C) goto L_81BB7E;  /* 81BB78 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BB7A lda */
    goto L_81BB88;  /* 81BB7C bra */
L_81BB7E:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BB7E lda */
    iny16();  /* 81BB80 iny */
    if (!IF_Z) goto L_81BB88;  /* 81BB81 bne */
    ldy16_v(0x8000);  /* 81BB83 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BB86 inc */
L_81BB88:
    sta8(ea_absx(0x0001));  /* 81BB88 sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BB8B asl */
    if (IF_C) goto L_81BB93;  /* 81BB8D bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BB8F lda */
    goto L_81BB9D;  /* 81BB91 bra */
L_81BB93:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BB93 lda */
    iny16();  /* 81BB95 iny */
    if (!IF_Z) goto L_81BB9D;  /* 81BB96 bne */
    ldy16_v(0x8000);  /* 81BB98 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BB9B inc */
L_81BB9D:
    sta8(ea_absx(0x0002));  /* 81BB9D sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BBA0 asl */
    if (IF_C) goto L_81BBA8;  /* 81BBA2 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BBA4 lda */
    goto L_81BBB2;  /* 81BBA6 bra */
L_81BBA8:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BBA8 lda */
    iny16();  /* 81BBAA iny */
    if (!IF_Z) goto L_81BBB2;  /* 81BBAB bne */
    ldy16_v(0x8000);  /* 81BBAD ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BBB0 inc */
L_81BBB2:
    sta8(ea_absx(0x0003));  /* 81BBB2 sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BBB5 asl */
    if (IF_C) goto L_81BBBD;  /* 81BBB7 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BBB9 lda */
    goto L_81BBC7;  /* 81BBBB bra */
L_81BBBD:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BBBD lda */
    iny16();  /* 81BBBF iny */
    if (!IF_Z) goto L_81BBC7;  /* 81BBC0 bne */
    ldy16_v(0x8000);  /* 81BBC2 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BBC5 inc */
L_81BBC7:
    sta8(ea_absx(0x0004));  /* 81BBC7 sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BBCA asl */
    if (IF_C) goto L_81BBD2;  /* 81BBCC bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BBCE lda */
    goto L_81BBDC;  /* 81BBD0 bra */
L_81BBD2:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BBD2 lda */
    iny16();  /* 81BBD4 iny */
    if (!IF_Z) goto L_81BBDC;  /* 81BBD5 bne */
    ldy16_v(0x8000);  /* 81BBD7 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BBDA inc */
L_81BBDC:
    sta8(ea_absx(0x0005));  /* 81BBDC sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BBDF asl */
    if (IF_C) goto L_81BBE7;  /* 81BBE1 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BBE3 lda */
    goto L_81BBF1;  /* 81BBE5 bra */
L_81BBE7:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BBE7 lda */
    iny16();  /* 81BBE9 iny */
    if (!IF_Z) goto L_81BBF1;  /* 81BBEA bne */
    ldy16_v(0x8000);  /* 81BBEC ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BBEF inc */
L_81BBF1:
    sta8(ea_absx(0x0006));  /* 81BBF1 sta */
    MEM8(asl, ea_dp(UNPACK_FLAGS));  /* 81BBF4 asl */
    if (IF_C) goto L_81BBFC;  /* 81BBF6 bcs */
    lda8(ea_dp(UNPACK_FILL));  /* 81BBF8 lda */
    goto L_81BC06;  /* 81BBFA bra */
L_81BBFC:
    lda8(ea_dpily(UNPACK_SOURCE));  /* 81BBFC lda */
    iny16();  /* 81BBFE iny */
    if (!IF_Z) goto L_81BC06;  /* 81BBFF bne */
    ldy16_v(0x8000);  /* 81BC01 ldy */
    MEM8(inc, ea_dp(UNPACK_SOURCE + 2));  /* 81BC04 inc */
L_81BC06:
    sta8(ea_absx(0x0007));  /* 81BC06 sta */
    op_rep(0x20);  /* 81BC09 rep */
    clc();  /* 81BC0B clc */
    txa16();  /* 81BC0C txa */
    adc16_v(0x0008);  /* 81BC0D adc */
    tax16();  /* 81BC10 tax */
    MEM16(dec, ea_dp(UNPACK_GROUPS));  /* 81BC11 dec */
    op_sep(0x20);  /* 81BC13 sep */
    return;  /* 81BC15 rts */
}

